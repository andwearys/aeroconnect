# AeroGrow Controller Demo

This is a demo web app for your ESP32-powered Aeroponics Controller.

## Features
- Responsive, mobile-friendly dashboard
- Modern neon dark UI (see css/style.css)
- Simulates sensor and control data (API PHP stubs)
- Easily connects to real ESP32 backend (just replace API stubs)

## Usage

1. **Install locally:**
   - Place files on any PHP-enabled server (XAMPP, WAMP, etc.).
   - Open `index.html` in your browser.

2. **Simulated Data:**
   - All API requests return demo data.
   - Control actions (misting, dosing, calibrate) are simulated.

3. **Replace with ESP32:**
   - Update `api/*.php` to connect to your ESP32 (REST, WebSocket, etc.).

## File Structure

```
aerogrow/
├── index.html
├── css/
│   └── style.css
├── js/
│   ├── app.js
│   └── esp32.js
├── api/
│   ├── config.php
│   ├── get_data.php
│   ├── set_control.php
│   └── get_settings.php
└── README.md
```

## Next Steps
- Connect real sensor/control logic to API endpoints
- Polish UI and add more features (user management, detailed reports, etc.)

---

**Questions or need help? Just ask!**