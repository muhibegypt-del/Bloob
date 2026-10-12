// Read-only snapshot from the webOS app inspector forwarded by ares-inspect.
const http = require('http');

const port = Number(process.argv[2] || 3600);
const expression = process.argv[3] === 'probe'
  ? '(async()=>{const m=await fetch("/media/big-buck-bunny-480p-30sec.mp4",{headers:{Range:"bytes=0-1023"}});const s=await fetch("/api/sources/search",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({type:"movie",id:"tt1254207"})});const j=await s.json();const a=await (await fetch("/api/account")).json();return JSON.stringify({mediaStatus:m.status,mediaRange:m.headers.get("content-range"),searchStatus:s.status,sources:j.sources?.length,paired:a.paired,error:j.error})})()'
  : process.argv[3] === 'state'
  ? 'JSON.stringify({player:!!document.querySelector("video"),sources:document.querySelector(".sources-screen h1")?.innerText,details:!!document.querySelector(".details-body"),focus:document.querySelector(".focused")?.innerText?.slice(0,100),hud:document.querySelector(".player-hud")?.innerText,video:(()=>{const v=document.querySelector("video");return v&&{time:v.currentTime,paused:v.paused,ready:v.readyState}})()})'
  : process.argv[3] === 'media'
  ? 'JSON.stringify((()=>{const v=document.querySelector("video");if(!v)return {player:false};const ranges=r=>Array.from({length:r.length},(_,i)=>[r.start(i),r.end(i)]);return {player:true,time:v.currentTime,duration:v.duration,paused:v.paused,ready:v.readyState,network:v.networkState,seeking:v.seeking,seekable:ranges(v.seekable),buffered:ranges(v.buffered),hud:document.querySelector(".player-hud")?.innerText}})())'
  : process.argv[3] === 'tracks'
  ? 'JSON.stringify((()=>{const v=document.querySelector("video");if(!v)return {player:false};const tracks=(list,keys)=>list?Array.from({length:list.length},(_,i)=>Object.fromEntries(keys.map(k=>[k,list[i][k]]))):[];return {audio:tracks(v.audioTracks,["id","label","language","enabled","kind"]),text:tracks(v.textTracks,["id","label","language","mode","kind"]),webkitAudio:!!v.webkitAudioDecodedByteCount}})())'
  : 'JSON.stringify({url:location.href,title:document.title,text:document.querySelector("#root")?.innerText?.slice(0,500),children:document.querySelector("#root")?.children.length})';
http.get(`http://localhost:${port}/json/list`, response => {
  let body = '';
  response.on('data', chunk => { body += chunk; });
  response.on('end', () => {
    const target = JSON.parse(body)[0];
    const ws = new WebSocket(target.webSocketDebuggerUrl);
    ws.onopen = () => ws.send(JSON.stringify({
      id: 1,
      method: 'Runtime.evaluate',
      params: {
        expression,
        returnByValue: true,
        awaitPromise: true,
      },
    }));
    ws.onmessage = event => {
      const message = JSON.parse(event.data);
      if (message.id === 1) {
        process.stdout.write(String(message.result?.result?.value || message.error || 'No result') + '\n');
        ws.close();
        setTimeout(() => process.exit(), 100);
      }
    };
    ws.onerror = error => { console.error(error); process.exitCode = 1; };
  });
}).on('error', error => { console.error(error); process.exitCode = 1; });
