import {VoiceMediaSession} from './voice-media.js';
const $=id=>document.getElementById(id),host=window.chrome?.webview;
let endpoint='',receiving=false,state=null,deviceId='',privateTab=false,joining=null,creating=false;
let localTest=null,testEpoch=0,testPending=false;
const names={proximity:'Cercanos',party:'Party',guild:'Guild',room:'Sala personal'};
const errorText={'connection-failed':'No se pudo conectar la voz','microphone-denied':'Permiso del micrófono denegado','microphone-unavailable':'No se pudo abrir el micrófono','room-full':'La sala está llena','room-unavailable':'No se pudo abrir esa sala','room-access-denied':'PIN incorrecto o demasiados intentos. Espera un minuto.'};
function post(type,data={}){host?.postMessage({type,...data});}
function status(text,error=false){$('status').textContent=text;document.body.classList.toggle('error',error);}
function view(name){for(const id of ['chat','rooms','create'])$(`${id}-view`).hidden=id!==name;$('panel-title').textContent=({chat:'CHAT DE VOZ',rooms:'SALAS DE VOZ',create:'CREAR SALA'})[name];}
function displayLevel(value,talking){
 document.body.classList.toggle('transmitting',talking);
 const thresholds=[.006,.02,.055,.12];[...$('level').children].forEach((bar,i)=>{const active=value>=thresholds[i];bar.style.opacity=active?'1':'.3';bar.style.background=active?'#55e58b':'#52644c';});
 $('input-state').textContent=localTest?'Prueba':!media.settings.microphone?'Apagado':talking?'Enviando':'Listo';
}
const media=new VoiceMediaSession({
 status(value){
  if(value==='off'||value==='disconnected'){state=null;document.body.classList.remove('connected','microphone-on');$('activate-mic').disabled=true;}
  if(value==='connected'){document.body.classList.add('connected');$('activate-mic').disabled=false;$('retry').hidden=true;status('Voz conectada');}
  else if(value==='connecting')status('Conectando recepción…');
  else if(value==='disconnected'){status('Voz desconectada',true);$('retry').hidden=false;receiving=false;$('enabled').checked=false;post('revoke');}
  else if(!receiving)status('Recepción desactivada');
  refreshAudio();
 },
 error(code){status(errorText[code]??'No se pudo completar la solicitud',true);if(!media.state)$('retry').hidden=false;refreshAudio();},
 level(value,talking){if(!localTest)displayLevel(value,talking);},
 state(value){
  state=value;
  $('current-room').textContent=`${value.channel.kind==='room'?(value.customRooms??[]).find(room=>room.id===value.channel.room)?.name??'Sala personal':names[value.channel.kind]} (${value.peers.length+1})`;
  document.querySelector('[data-channel="party"]').disabled=!value.party;document.querySelector('[data-channel="guild"]').disabled=!value.guild;
  document.querySelectorAll('[data-channel]').forEach(button=>button.classList.toggle('selected',button.dataset.channel===value.channel.kind));$('show-rooms').classList.toggle('selected',value.channel.kind==='room');
  $('guild-controls').hidden=value.channel.kind!=='guild';
  $('guild-room').replaceChildren(...value.rooms.map(room=>new Option(room.name||'Canal principal de guild',room.id)));$('guild-room').value=value.channel.room;
  $('peer-count').textContent=String(value.peers.length);
  $('peers').replaceChildren(...value.peers.map(peer=>{
   const row=document.createElement('li'),name=document.createElement('span'),mute=document.createElement('button');name.textContent=peer.name;mute.textContent=media.muted.has(peer.id)?'Activar':'Silenciar';
   mute.onclick=()=>{media.mute(peer.id,!media.muted.has(peer.id));mute.textContent=media.muted.has(peer.id)?'Activar':'Silenciar';row.classList.toggle('muted',media.muted.has(peer.id));};row.append(name,mute);return row;
  }));
  if(value.channel.kind==='room'&&(creating||joining===value.channel.room)){creating=false;joining=null;view('chat');}
  renderRooms();refreshAudio();
 },
});
function refreshAudio(){
 const mic=!!media.settings.microphone;document.body.classList.toggle('microphone-on',mic);post('microphoneState',{enabled:mic});
 $('activate-mic').textContent=mic?'APAGAR MICRÓFONO':'ACTIVAR MICRÓFONO';
 $('send-state').textContent=!mic?'Solo escuchas: nadie te oye.':!media.settings.focused?'Micrófono en pausa: cliente en segundo plano.':media.settings.mode==='ptt'?'Mantén K en el cliente activo para hablar.':'Micrófono activo: te oyen en este canal.';
 $('receive-state').textContent=!state?'Recepción desconectada':media.settings.deafen?'Recepción silenciada':({proximity:'Escuchando a los cercanos',party:'Escuchando a tu party',guild:'Escuchando a tu guild',room:'Escuchando esta sala'})[state.channel.kind];
 $('talk').hidden=media.settings.mode!=='ptt'||!mic;
 if(!mic&&!localTest)displayLevel(0,false);
}
function beginListening(){
 if(!host||!endpoint){status('Abre este panel desde el cliente del juego',true);return;}
 if(receiving&&media.socket)return;
 receiving=true;$('enabled').checked=true;$('retry').hidden=true;status('Solicitando sesión al servidor…');post('requestTicket');
}
function stopListening(){receiving=false;$('enabled').checked=false;state=null;void media.stop();post('revoke');refreshAudio();}
$('enabled').onchange=()=>{$('enabled').checked?beginListening():stopListening();};$('retry').onclick=()=>{stopListening();beginListening();};
async function toggleMicrophone(){
 if(!state){status('Espera a que conecte la recepción',true);return;}
 $('activate-mic').disabled=true;
 try{await stopLocalTest();await media.setMicrophoneEnabled(!media.settings.microphone,deviceId);if(media.settings.microphone)void listDevices();}
 catch{status('No se pudo abrir el micrófono. Revisa el permiso y el dispositivo.',true);}
 finally{$('activate-mic').disabled=!state;refreshAudio();}
}
$('activate-mic').onclick=toggleMicrophone;$('mic-symbol').onclick=toggleMicrophone;
for(const id of ['gain','volume'])$(id).oninput=()=>{$(`${id}-value`).textContent=`${$(id).value}%`;$(id).style.setProperty('--fill',`${$(id).value}%`);media.setSettings({[id]:Number($(id).value)/100});};
$('deafen').onclick=()=>{const deafen=!media.settings.deafen;media.setSettings({deafen});$('deafen').textContent=deafen?'Activar sonido':'Silenciar';$('deafen').setAttribute('aria-pressed',String(deafen));refreshAudio();};
$('mode').onchange=()=>{media.setSettings({mode:$('mode').value});refreshAudio();};
$('talk').onpointerdown=event=>{$('talk').setPointerCapture(event.pointerId);media.setSettings({talk:true});$('talk').classList.add('held');};
for(const event of ['pointerup','pointercancel','lostpointercapture'])$('talk').addEventListener(event,()=>{media.setSettings({talk:false});$('talk').classList.remove('held');});
document.querySelectorAll('[data-channel]').forEach(button=>button.onclick=()=>{if(state)media.select(button.dataset.channel);else status('Espera a que conecte la recepción');});
$('guild-room').onchange=()=>media.select('guild',$('guild-room').value);
$('show-rooms').onclick=()=>{view('rooms');renderRooms();};$('back-chat').onclick=()=>view('chat');
function roomTab(value){privateTab=value;$('public-rooms').classList.toggle('selected',!value);$('private-rooms').classList.toggle('selected',value);joining=null;$('join-form').hidden=true;renderRooms();}
$('public-rooms').onclick=()=>roomTab(false);$('private-rooms').onclick=()=>roomTab(true);
function renderRooms(){
 const all=state?.customRooms??[],rooms=all.filter(room=>room.locked===privateTab);
 $('public-rooms').textContent=`Públicas ${all.filter(room=>!room.locked).length+1}`;$('private-rooms').textContent=`Privadas ${all.filter(room=>room.locked).length}`;
 const rows=rooms.map(room=>{
  const li=document.createElement('li'),button=document.createElement('button'),name=document.createElement('span'),count=document.createElement('span');
  name.textContent=(room.locked?'🔒 ':'● ')+room.name;count.textContent=`${room.count} / ${room.capacity}`;button.append(name,count);button.classList.toggle('selected',state?.channel.kind==='room'&&state.channel.room===room.id);
  button.onclick=()=>{if(!state)return;joining=room.id;if(room.locked){$('join-form').hidden=false;$('join-pin').value='';$('join-pin').focus();}else media.joinPersonalRoom(room.id,'');};li.append(button);return li;
 });
 if(!privateTab){const li=document.createElement('li'),button=document.createElement('button');button.textContent='● Cercanos · canal automático';button.classList.toggle('selected',state?.channel.kind==='proximity');button.onclick=()=>{if(state){media.select('proximity');view('chat');}};li.append(button);rows.unshift(li);}
 $('room-list').replaceChildren(...rows);$('empty-rooms').hidden=rows.length>0;
}
$('join-form').onsubmit=event=>{event.preventDefault();if(joining){media.joinPersonalRoom(joining,$('join-pin').value);$('join-pin').value='';}};
$('show-create').onclick=()=>{if(!state){status('Conecta la recepción antes de crear una sala',true);return;}$('room-private').checked=privateTab;updatePrivacy();view('create');};
function updatePrivacy(){$('pin-fields').hidden=!$('room-private').checked;$('room-pin').required=$('room-private').checked;if(!$('room-private').checked)$('room-pin').value='';}
$('room-private').onchange=updatePrivacy;$('cancel-create').onclick=()=>{$('room-pin').value='';view('rooms');};
$('room-form').onsubmit=event=>{event.preventDefault();if(!state)return;creating=true;media.createPersonalRoom($('room-name').value.trim(),$('room-private').checked?$('room-pin').value:'');$('room-pin').value='';};
async function listDevices(){try{const devices=await navigator.mediaDevices.enumerateDevices();$('microphone').replaceChildren(new Option('Micrófono predeterminado',''),...devices.filter(device=>device.kind==='audioinput').map(device=>new Option(device.label||'Micrófono',device.deviceId)));$('microphone').value=deviceId;}catch{}}
$('microphone').onchange=async()=>{deviceId=$('microphone').value;try{await media.replaceMicrophone(deviceId);}catch{status('No se pudo cambiar el micrófono',true);}};
async function stopLocalTest(){++testEpoch;const previous=localTest;localTest=null;clearInterval(previous?.timer);previous?.stream.getTracks().forEach(track=>track.stop());if(previous?.context.state!=='closed')await previous?.context.close();$('test-microphone').textContent='Probar mi micrófono';displayLevel(0,false);}
$('test-microphone').onclick=async()=>{
 if(localTest||testPending){await stopLocalTest();status('Prueba local detenida');return;}
 await media.setMicrophoneEnabled(false);refreshAudio();const epoch=++testEpoch;testPending=true;let context,stream;
 try{context=new AudioContext();await context.resume();stream=await navigator.mediaDevices.getUserMedia({audio:{deviceId:deviceId?{exact:deviceId}:undefined,echoCancellation:true,noiseSuppression:true},video:false});if(epoch!==testEpoch){stream.getTracks().forEach(track=>track.stop());await context.close();return;}
  const source=context.createMediaStreamSource(stream),gain=context.createGain(),analyser=context.createAnalyser();gain.gain.value=.5;source.connect(gain);gain.connect(context.destination);source.connect(analyser);analyser.fftSize=512;const samples=new Float32Array(512);
  const timer=setInterval(()=>{analyser.getFloatTimeDomainData(samples);displayLevel(Math.sqrt(samples.reduce((sum,sample)=>sum+sample*sample,0)/samples.length),false);},80);localTest={context,stream,timer};$('test-microphone').textContent='Detener prueba local';status('Prueba local: no transmite a otros');void listDevices();
 }catch{stream?.getTracks().forEach(track=>track.stop());if(context?.state!=='closed')await context?.close();status('No se pudo abrir el micrófono',true);}finally{testPending=false;}
};
function minimize(value){if(value&&localTest)void stopLocalTest();$('voice-panel').hidden=value;post('minimize',{value});}
$('minimize').onclick=()=>minimize(true);$('drag-handle').onpointerdown=event=>{if(!event.target.closest('button'))post('drag');};
host?.addEventListener('message',event=>{
 const message=event.data;
 if(message.type==='config'){endpoint=message.endpoint;minimize(!!message.minimized);beginListening();void listDevices();post('ready');}
 else if(message.type==='control'){media.setSettings({focused:message.focused,ptt:message.ptt});refreshAudio();}
 else if(message.type==='ticket'&&receiving&&/^[a-f0-9]{64}$/.test(message.ticket))void media.start({endpoint,ticket:message.ticket});
 else if(message.type==='unavailable'){stopListening();status('El servidor no pudo autorizar la sesión',true);$('retry').hidden=false;}
 else if(message.type==='restore')minimize(false);
 else if(message.type==='diagnostics')void media.diagnostics().then(peers=>post('diagnostics',{metrics:{at:Date.now(),connected:!!media.state,uiConnected:!!state,context:media.context?.state??'closed',error:media.lastError??null,microphone:!!media.input,transmitting:!!media.transmitting,volume:media.settings.volume,deafen:media.settings.deafen,peers:peers.map(({state,bytesReceived,bytesSent,audioEnergy,sourceEnergy,outgoingEnabled,incomingMuted,playback})=>({state,bytesReceived,bytesSent,audioEnergy,sourceEnergy,outgoingEnabled,incomingMuted,playback}))}}));
});
document.addEventListener('keydown',event=>{if(event.code==='KeyK'&&!event.target.matches('input,select,textarea')&&!event.repeat)media.setSettings({talk:true});});document.addEventListener('keyup',event=>{if(event.code==='KeyK')media.setSettings({talk:false});});window.addEventListener('blur',()=>media.setSettings({talk:false,ptt:false}));window.addEventListener('pagehide',()=>{void stopLocalTest();void media.stop();post('revoke');});
$('test-connection').onclick=async()=>{status('Enviando tono de prueba al otro cliente…');if(!await media.testConnectionTone())status('Conecta otro jugador y apaga el micrófono para esta prueba',true);else status('Tono de prueba terminado');};
post('loaded');
document.addEventListener('pointerdown',()=>{if(media.context?.state==='suspended')void media.context.resume();},{capture:true});
