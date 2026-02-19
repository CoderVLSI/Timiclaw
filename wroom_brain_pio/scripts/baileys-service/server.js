/**
 * TimiClaw WhatsApp Service (Baileys)
 *
 * A simple HTTP-to-WhatsApp gateway using @whiskeysockets/baileys
 *
 * DEPLOYMENT:
 * - Railway: https://railway.app/template
 * - Render: https://render.com
 * - Your VPS: node server.js
 *
 * ENV VARS:
 * - PORT=3000
 * - API_KEY=your_secret_key
 */

const { default: makeWASocket, useMultiFileAuthState, DisconnectReason } = require('@whiskeysockets/baileys');
const { Boom } = require('@hapi/boom');
const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;
const API_KEY = process.env.API_KEY || '';

// Auth state persistence
const AUTH_PATH = path.join(__dirname, 'auth_info_baileys');

function validateApiKey(req, res, next) {
  const key = req.headers['authorization']?.replace('Bearer ', '') || req.headers['x-api-key'];
  if (API_KEY && key !== API_KEY) {
    return res.status(401).json({ error: 'Invalid API key' });
  }
  next();
}

app.use(bodyParser.json({ limit: '10mb' }));

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok', connected: sock?.user ?? false });
});

// Send text message
app.post('/send-message', validateApiKey, async (req, res) => {
  try {
    const { message, to } = req.body;

    if (!message) {
      return res.status(400).json({ error: 'Missing message field' });
    }

    if (!sock) {
      return res.status(503).json({ error: 'WhatsApp not connected. Scan QR code first.' });
    }

    // Send to own saved messages (like "Note to Self")
    // You can modify this to send to specific contacts
    const jid = to || 'status@broadcast'; // or use a specific JID

    await sock.sendMessage(jid, { text: message });

    res.json({ success: true, message: 'Message sent' });
  } catch (error) {
    console.error('Error sending message:', error);
    res.status(500).json({ error: error.message });
  }
});

// Send web files (HTML, CSS, JS)
app.post('/send-files', validateApiKey, async (req, res) => {
  try {
    const { topic, html, css, js } = req.body;

    if (!html) {
      return res.status(400).json({ error: 'Missing html field' });
    }

    if (!sock) {
      return res.status(503).json({ error: 'WhatsApp not connected. Scan QR code first.' });
    }

    // Create combined HTML file
    const fullHtml = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>${topic || 'Generated Files'}</title>
  <style>${css || ''}</style>
</head>
<body>
${html}
<script>${js || ''}</script>
</body>
</html>`;

    // Send as document
    const buffer = Buffer.from(fullHtml, 'utf-8');

    await sock.sendMessage('status@broadcast', {
      document: buffer,
      mimetype: 'text/html',
      fileName: `${topic || 'website'}.html`,
      caption: `📄 Generated files for: ${topic || 'website'}`
    });

    res.json({ success: true, message: 'Files sent' });
  } catch (error) {
    console.error('Error sending files:', error);
    res.status(500).json({ error: error.message });
  }
});

// WhatsApp connection
let sock;

async function connectToWhatsApp() {
  const { state, saveCreds } = await useMultiFileAuthState(AUTH_PATH);

  sock = makeWASocket({
    auth: state,
    printQRInTerminal: true,
  });

  sock.ev.on('connection.update', (update) => {
    const { connection, lastDisconnect, qr } = update;

    if (qr) {
      console.log('QR CODE:', qr);
      console.log('Scan this with your WhatsApp mobile:');
    }

    if (connection === 'close') {
      const shouldReconnect = (lastDisconnect.error instanceof Boom)?.output?.statusCode !== DisconnectReason.loggedOut;
      console.log('Connection closed. Reconnecting:', shouldReconnect);

      if (shouldReconnect) {
        connectToWhatsApp();
      }
    } else if (connection === 'open') {
      console.log('WhatsApp connection opened!');
    }
  });

  sock.ev.on('creds.update', saveCreds);
}

// Start server
app.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
  console.log(`Health: http://localhost:${PORT}/health`);
  connectToWhatsApp();
});
