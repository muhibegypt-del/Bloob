// Drives only this app through its web inspector and reads HTMLMediaElement state.
const http = require('http');
const port = Number(process.argv[2] || 3600);
const wait = ms => new Promise(resolve => setTimeout(resolve, ms));

http.get(`http://localhost:${port}/json/list`, response => {
  let body = '';
  response.on('data', chunk => { body += chunk; });
  response.on('end', () => {
    const target = JSON.parse(body)[0];
    const ws = new WebSocket(target.webSocketDebuggerUrl);
    let nextId = 1;
    const pending = new Map();
    ws.onmessage = event => {
      const message = JSON.parse(event.data);
      if (!pending.has(message.id)) return;
      pending.get(message.id)(message);
      pending.delete(message.id);
    };
    const send = (method, params) => new Promise(resolve => {
      const id = nextId++;
      pending.set(id, resolve);
      ws.send(JSON.stringify({ id, method, params }));
    });
    const evaluate = async expression => {
      const result = await send('Runtime.evaluate', { expression, returnByValue: true });
      return result.result && result.result.result && result.result.result.value;
    };
    ws.onopen = async () => {
      try {
        if (process.argv[3] === 'reveal') {
          await send('Input.dispatchKeyEvent', { type: 'keyDown', key: 'ArrowUp', code: 'ArrowUp', windowsVirtualKeyCode: 38, nativeVirtualKeyCode: 38 });
          await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'ArrowUp', code: 'ArrowUp', windowsVirtualKeyCode: 38, nativeVirtualKeyCode: 38 });
          await wait(250);
          console.log(await evaluate('JSON.stringify({clock:document.querySelector("video")?.currentTime,duration:document.querySelector("video")?.duration,hud:document.querySelector(".player-hud")?.innerText})'));
          ws.close();
          return setTimeout(() => process.exit(), 100);
        }
        if (process.argv[3] === 'featured') {
          const key = async (name, code) => {
            await send('Input.dispatchKeyEvent', { type: 'keyDown', key: name, code: name, windowsVirtualKeyCode: code, nativeVirtualKeyCode: code });
            await send('Input.dispatchKeyEvent', { type: 'keyUp', key: name, code: name, windowsVirtualKeyCode: code, nativeVirtualKeyCode: code });
            await wait(350);
          };
          if (await evaluate('!!document.querySelector(".details-body")')) await key('Backspace', 8);
          await key('ArrowDown', 40);
          await key('ArrowDown', 40);
          await key('Enter', 13);
          await key('Enter', 13);
          await wait(3000);
          console.log(await evaluate('JSON.stringify({title:document.querySelector(".sources-screen h1")?.innerText,searching:document.querySelector(".source-divider")?.innerText,results:document.querySelectorAll(".discovered-list .source-option").length,message:document.querySelector(".source-message")?.innerText})'));
          ws.close();
          return setTimeout(() => process.exit(), 100);
        }
        if (process.argv[3] === 'exit') {
          await send('Input.dispatchKeyEvent', { type: 'keyDown', key: 'Backspace', code: 'Backspace', windowsVirtualKeyCode: 8, nativeVirtualKeyCode: 8 });
          await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Backspace', code: 'Backspace', windowsVirtualKeyCode: 8, nativeVirtualKeyCode: 8 });
          await wait(500);
          console.log(await evaluate('JSON.stringify({playerVisible:!!document.querySelector("video"),sourcePicker:!!document.querySelector(".sources-screen")})'));
          ws.close();
          return setTimeout(() => process.exit(), 100);
        }
        const start = await evaluate('document.querySelector(".sources-screen") ? "sources" : document.querySelector(".details-body") ? "details" : document.querySelector(".hero") ? "home" : "unknown"');
        if (start !== 'home') throw new Error(`Expected Home, found ${start}`);
        for (let i = 0; i < 3; i++) {
          await send('Input.dispatchKeyEvent', { type: 'keyDown', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13, nativeVirtualKeyCode: 13 });
          await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13, nativeVirtualKeyCode: 13 });
          await wait(500);
        }
        await wait(6000);
        console.log(await evaluate('JSON.stringify((()=>{const v=document.querySelector("video");return {screen:!!v,src:v?.currentSrc,paused:v?.paused,time:v?.currentTime,readyState:v?.readyState,networkState:v?.networkState,errorCode:v?.error?.code,state:document.querySelector(".player-state")?.innerText}})())'));
      } catch (error) { console.error(error.message); process.exitCode = 1; }
      ws.close();
      setTimeout(() => process.exit(), 100);
    };
    ws.onerror = error => { console.error(error); process.exitCode = 1; };
  });
}).on('error', error => { console.error(error); process.exitCode = 1; });
