ESP32 Spotify OLED Display
A standalone, Wi-Fi connected Spotify "Now Playing" desktop companion built using an **ESP32** and a **0.96" SSD1306 OLED Display (128x64 I2C)**. 
It communicates directly with the **Spotify Web API** using OAuth 2.0 to stream live playback metadata, displaying a dynamic Play/Pause status icon, auto-scrolling track titles, artist details, and a live progress bar.
Features
- **Dynamic Status Indicator**: Displays custom 16x16 pixel bitmaps for Play (▶) and Pause (❚❚) states that toggle in real-time.
- **Smooth Marquee Scrolling**: Automatically scrolls track titles that exceed the 128px screen width.
- **Track Progress Bar**: Real-time visual progress bar showing elapsed vs. total track duration.
- **Zero-Maintenance Auth**: Uses a permanent Spotify `Refresh Token` to automatically generate and renew temporary 1-hour access tokens in the background without needing user intervention.
- **100% Standalone**: Runs independently off any 5V USB wall adapter or power bank—no PC connection or companion app required after uploading!
isplay Preview
```text
+------------------------------------+
|               [ ▶ ]                |  <-- Live Play / Pause State Icon
|------------------------------------|
| Blinding Lights                    |  <-- Smooth Marquee Scrolling Title
| The Weeknd                         |  <-- Artist Name
| [==========----------------------] |  <-- Dynamic Track Progress Bar
+------------------------------------+
Hardware Requirements
| Component | Quantity | Notes |
| :--- | :--- | :--- |
| **ESP32 Development Board** | 1 | NodeMCU-32S, ESP32 Dev Module, or equivalent |
| **0.96" I2C OLED Display** | 1 | SSD1306 controller (128x64 resolution) |
| **Jumper Wires** | 4 | Female-to-Female |
| **Micro-USB / USB-C Cable** | 1 | For flashing and power |
iring Diagram
Connect the 0.96" OLED display to the ESP32 using the default I2C pins:
| 0.96" OLED Pin | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **GND** | **GND** | Ground |
| **VCC** | **3.3V** (or 5V) | Power |
| **SCL** | **GPIO 22** | I2C Clock |
| **SDA** | **GPIO 21** | I2C Data |
**Software & Library Dependencies
Install the following libraries using the **Arduino IDE Library Manager** (`Ctrl + Shift + I`):
1. **`Adafruit SSD1306`** (by Adafruit) — *Click "Install All" to include dependencies*
2. **`Adafruit GFX Library`** (by Adafruit)
3. **`ArduinoJson`** (by Benoît Blanchon) — *Version 6.x or 7.x*
**
Spotify API Setup (One-Time Setup)
### 1. Create a Spotify Developer App
1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard) and log in.
2. Click **Create app** and configure:
   - **App Name**: `ESP32 Spotify Display`
   - **App Description**: `ESP32 OLED Spotify Player`
   - **Redirect URIs**: `https://oauth.pstmn.io/v1/browser-callback` *(Click **Add**)*
   - **Which API/SDKs are you planning to use?**: Check **Web API**
3. Save the app, click **Settings**, and copy your **Client ID** and **Client Secret** (click *View client secret*).
2. Authorize and Get Authorization Code
1. Open your browser and paste this URL (replace `YOUR_CLIENT_ID` with your real Client ID):
   ```text
   https://accounts.spotify.com/authorize?client_id=YOUR_CLIENT_ID&response_type=code&redirect_uri=https://oauth.pstmn.io/v1/browser-callback&scope=user-read-playback-state%20user-read-currently-playing
   ```
Click **Agree**.
3. You will be redirected to a page saying *"Authentication complete"*. 
4. Look at your browser's **URL address bar** and copy the code string between `?code=` and `#_=_`:
   ```text
   https://oauth.pstmn.io/v1/browser-callback?code=COPY_THIS_CODE_HERE#_=_
3. Generate Your Permanent Refresh Token
Open [ReqBin](https://reqbin.com/post-online) (or use PowerShell / curl):
- **Method**: `POST`
- **URL**: `https://accounts.spotify.com/api/token`
- **Content-Type**: `URL-Encoded`
- **Parameters**:
  - `grant_type`: `authorization_code`
  - `redirect_uri`: `https://oauth.pstmn.io/v1/browser-callback`
  - `code`: *(Your code from Step 2)*
  - `client_id`: *(Your 32-character Client ID)*
  - `client_secret`: *(Your 32-character Client Secret)*
Click **Send**. In the response on the right, copy your permanent **`refresh_token`**.
Installation & Flashing
