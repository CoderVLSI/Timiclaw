# TimiClaw WhatsApp Service (Baileys)

HTTP-to-WhatsApp gateway for TimiClaw ESP32 firmware.

## Features

- Send text messages via WhatsApp
- Send generated web files (HTML, CSS, JS) as documents
- QR code authentication (no phone number needed)
- REST API with API key protection

## Deployment Options

### Option 1: Railway (Recommended)

1. Fork this repo or create a new one with `server.js` and `package.json`
2. Go to [railway.app](https://railway.app)
3. Click "New Project" → "Deploy from GitHub"
4. Select your repo
5. Add environment variables:
   - `API_KEY`: Your secret key (optional but recommended)
6. Deploy!
7. Get your service URL (e.g., `https://timiclaw-wa.up.railway.app`)
8. Add to TimiClaw `.env`:
   ```
   WHATSAPP_ENDPOINT_URL=https://timiclaw-wa.up.railway.app
   WHATSAPP_API_KEY=your_secret_key
   ```

### Option 2: Render

1. Go to [render.com](https://render.com)
2. Click "New" → "Web Service"
3. Connect your GitHub repo
4. Set:
   - Runtime: `Node`
   - Build Command: `npm install`
   - Start Command: `node server.js`
5. Add environment variables
6. Deploy!

### Option 3: Your VPS/Server

```bash
# Clone the service files
git clone <your-repo>
cd baileys-service

# Install dependencies
npm install

# Set environment variables
export API_KEY=your_secret_key
export PORT=3000

# Start
node server.js
```

## API Endpoints

### POST /send-message

Send a text message via WhatsApp.

**Request:**
```json
{
  "message": "Hello from TimiClaw!"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Message sent"
}
```

### POST /send-files

Send generated web files as HTML document.

**Request:**
```json
{
  "topic": "portfolio site",
  "html": "<html>...</html>",
  "css": "body { ... }",
  "js": "console.log('hi');"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Files sent"
}
```

### GET /health

Check service status.

**Response:**
```json
{
  "status": "ok",
  "connected": true
}
```

## First Time Setup

1. Deploy the service
2. Check the logs for QR code
3. Scan QR code with WhatsApp mobile app:
   - Open WhatsApp → Settings → Linked Devices
   - Tap "Link a Device"
   - Scan the QR code
4. Connection will be established!

## Usage from TimiClaw

```
# Send message
/whatsapp_send Hello from TimiClaw!

# Send web files
/whatsapp_send_files portfolio site for photographer

# Natural language
send this to whatsapp: check out my new portfolio
```

## Security

- Always set `API_KEY` environment variable
- Use HTTPS in production
- The service sends to "Note to Self" by default
- Modify `to` parameter in `server.js` to send to specific contacts

## Troubleshooting

**QR code not showing:**
- Check logs at your deployment platform
- Ensure the service is running

**Connection issues:**
- WhatsApp may disconnect after inactivity
- Service auto-reconnects

**Files not sending:**
- Check file size limits (WhatsApp: ~100MB for documents)
- Ensure HTML is valid UTF-8
