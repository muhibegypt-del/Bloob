// Captures only this prototype's rendered page through its local inspector.
const fs = require('fs');
const http = require('http');
const port = Number(process.argv[2]);
const destination = process.argv[3];
if (!port || !destination) throw new Error('Usage: node capture-player-tv.cjs <inspector-port> <output.png>');

http.get(`http://localhost:${port}/json/list`, response => {
  let body = '';
  response.on('data', chunk => { body += chunk; });
  response.on('end', () => {
    const ws = new WebSocket(JSON.parse(body)[0].webSocketDebuggerUrl);
    let id = 0;
    const pending = new Map();
    ws.onmessage = event => {
      const message = JSON.parse(event.data);
      if (!pending.has(message.id)) return;
      pending.get(message.id)(message);
      pending.delete(message.id);
    };
    const send = (method, params = {}) => new Promise(resolve => {
      const next = ++id;
      pending.set(next, resolve);
      ws.send(JSON.stringify({ id: next, method, params }));
    });
    ws.onopen = async () => {
      try {
        await send('Input.dispatchKeyEvent', { type: 'keyDown', key: 'ArrowUp', code: 'ArrowUp', windowsVirtualKeyCode: 38, nativeVirtualKeyCode: 38 });
        await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'ArrowUp', code: 'ArrowUp', windowsVirtualKeyCode: 38, nativeVirtualKeyCode: 38 });
        await new Promise(resolve => setTimeout(resolve, 300));
        const screenshot = await send('Page.captureScreenshot', { format: 'png' });
        if (!screenshot.result?.data) throw new Error(screenshot.error?.message || 'Screenshot unavailable');
        fs.writeFileSync(destination, Buffer.from(screenshot.result.data, 'base64'));
        console.log('Saved prototype screenshot');
      } catch (error) { console.error(error.message); process.exitCode = 1; }
      ws.close();
    };
  });
}).on('error', error => { console.error(error.message); process.exitCode = 1; });
