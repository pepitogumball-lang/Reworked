const http = require('http');
const fs = require('fs');
const path = require('path');

const PORT = 5000;
const HOST = '0.0.0.0';

const html = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Xd-rework - Geode Mod</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: #0d1117;
      color: #e6edf3;
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 2rem;
    }
    .card {
      background: #161b22;
      border: 1px solid #30363d;
      border-radius: 12px;
      padding: 2.5rem;
      max-width: 580px;
      width: 100%;
    }
    .badge {
      display: inline-block;
      background: #f6a623;
      color: #0d1117;
      font-size: 0.7rem;
      font-weight: 700;
      padding: 2px 10px;
      border-radius: 20px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      margin-bottom: 1rem;
    }
    h1 { font-size: 2rem; margin-bottom: 0.5rem; }
    .sub { color: #8b949e; margin-bottom: 1.5rem; font-size: 0.95rem; }
    .meta { display: flex; gap: 1rem; margin-bottom: 1.5rem; flex-wrap: wrap; }
    .meta-item {
      background: #21262d;
      border: 1px solid #30363d;
      border-radius: 6px;
      padding: 0.35rem 0.75rem;
      font-size: 0.8rem;
      color: #8b949e;
    }
    .meta-item span { color: #e6edf3; font-weight: 600; }
    h2 { font-size: 1rem; color: #8b949e; margin-bottom: 0.75rem; text-transform: uppercase; letter-spacing: 0.5px; }
    .features {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0.5rem;
      margin-bottom: 1.5rem;
    }
    .feature {
      background: #21262d;
      border-radius: 6px;
      padding: 0.5rem 0.75rem;
      font-size: 0.85rem;
      color: #c9d1d9;
    }
    .link {
      display: inline-block;
      margin-top: 1rem;
      color: #58a6ff;
      text-decoration: none;
      font-size: 0.9rem;
    }
    .link:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <div class="card">
    <div class="badge">Geode Mod</div>
    <h1>Xd-rework</h1>
    <p class="sub">A rework of xdBot — fully updated for modern Geode and GD versions.</p>
    <div class="meta">
      <div class="meta-item">Geode <span>v5.6.1</span></div>
      <div class="meta-item">GD <span>2.2081</span></div>
      <div class="meta-item">Developer <span>flinger-bit</span></div>
    </div>
    <h2>Features</h2>
    <div class="features">
      <div class="feature">Macro Recording</div>
      <div class="feature">Frame Fixes</div>
      <div class="feature">Speedhack</div>
      <div class="feature">Frame Stepper</div>
      <div class="feature">NoClip</div>
      <div class="feature">Trajectory Preview</div>
      <div class="feature">Coin Finder</div>
      <div class="feature">Layout Mode</div>
      <div class="feature">Clickbot</div>
      <div class="feature">Autoclicker</div>
      <div class="feature">Renderer</div>
      <div class="feature">Autosave</div>
    </div>
    <a class="link" href="https://github.com/flinger-bit/Xd-rework" target="_blank">View on GitHub →</a>
  </div>
</body>
</html>`;

const server = http.createServer((req, res) => {
  res.writeHead(200, { 'Content-Type': 'text/html' });
  res.end(html);
});

server.listen(PORT, HOST, () => {
  console.log(`Xd-rework project page running at http://${HOST}:${PORT}/`);
});
