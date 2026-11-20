// Independent WebRTC audio client. Credentials stay in memory and are never put in URLs.
export class VoiceMediaSession {
  constructor(events = {}) {
    this.events = events;
    this.generation = 0;
    this.peers = new Map();
    this.muted = new Set();
    this.settings = {gain:1,volume:1,deafen:false,mode:'open',focused:false,ptt:false,talk:false,microphone:false};
    this.messages = Promise.resolve();
  }

  async start({endpoint,ticket,deviceId='',capture}) {
    await this.stop();
    const generation = ++this.generation;
    this.events.status?.('connecting');
    try {
      this.context = new AudioContext();
      // Signaling must not wait indefinitely for a browser autoplay gesture.
      void this.context.resume().catch(()=>{});
      if (generation !== this.generation)return;
      this.sendGain = this.context.createGain();
      this.sendGain.gain.value = this.settings.gain;
      this.destination = this.context.createMediaStreamDestination();
      this.analyser = this.context.createAnalyser();
      this.analyser.fftSize = 256;
      this.meterBytes = new Float32Array(this.analyser.fftSize);
      this.sendGain.connect(this.destination);
      this.sendGain.connect(this.analyser);
      this.master = this.context.createGain();
      this.master.connect(this.context.destination);
      if(capture)await this.setMicrophoneEnabled(true,deviceId,capture);
      this.applySettings();
      this.socket = new WebSocket(endpoint);
      this.socket.onopen = () => {if (generation===this.generation)this.send({type:'auth',ticket});};
      this.socket.onmessage = event => {
        if(generation!==this.generation||typeof event.data!=='string'||event.data.length>131072)return;
        this.messages = this.messages.then(async()=>{
          if(generation!==this.generation)return;
          const message=JSON.parse(event.data);
          if(message.type==='state')await this.applyState(message);
          else if(message.type==='signal')await this.receiveSignal(message);
          else if(message.type==='error')this.events.error?.(message.code);
        }).catch(error=>{this.lastError={stage:'message',name:error?.name??'Error',location:error?.stack?.match(/voice-(?:ui|media)\.js:\d+:\d+/)?.[0]??''};this.events.error?.('connection-failed');});
      };
      this.socket.onclose = () => {if(generation===this.generation){void this.stop();this.events.status?.('disconnected');}};
      this.socket.onerror = () => this.events.error?.('connection-failed');
      this.meterTimer = setInterval(()=>this.measure(),100);
      this.deadline = setTimeout(()=>{if(generation===this.generation&&!this.state){void this.stop();this.events.error?.('connection-failed');}},8000);
    } catch(error) {
      if(generation!==this.generation)return;
      await this.stop();
      this.events.error?.(error?.name==='NotAllowedError'?'microphone-denied':'microphone-unavailable');
    }
  }

  async stop() {
    ++this.generation;
    clearInterval(this.meterTimer);
    clearTimeout(this.deadline);
    if(this.socket){this.socket.onclose=null;this.socket.close();this.socket=null;}
    for(const id of this.peers.keys())this.removePeer(id);
    this.input?.getTracks().forEach(track=>track.stop());
    this.destination?.stream.getTracks().forEach(track=>track.stop());
    this.input=null;
    this.settings.microphone=false;
    this.settings.testTone=false;
    this.captureEpoch=(this.captureEpoch??0)+1;
    this.state=null;
    const context=this.context;this.context=null;
    if(context&&context.state!=='closed')await context.close();
    this.events.level?.(0,false);
    this.events.status?.('off');
  }

  setSettings(update) {
    Object.assign(this.settings,update);
    this.settings.gain=Math.max(0,Math.min(1,this.settings.gain));
    this.settings.volume=Math.max(0,Math.min(1,this.settings.volume));
    this.applySettings();
  }

  applySettings() {
    const settings=this.settings;
    this.transmitting=!!this.state&&(settings.testTone||settings.microphone&&settings.focused&&(settings.mode==='open'||settings.ptt||settings.talk));
    for(const track of this.destination?.stream.getAudioTracks()??[])track.enabled=this.transmitting;
    if(this.sendGain)this.sendGain.gain.value=settings.gain;
    if(this.master)this.master.gain.value=settings.deafen?0:settings.volume;
    for(const peer of this.peers.values())this.updatePeerVolume(peer);
  }

  async replaceMicrophone(deviceId) {
    if(!this.context||!this.input)return;
    const generation=this.generation;
    const stream=await navigator.mediaDevices.getUserMedia({audio:{deviceId:deviceId?{exact:deviceId}:undefined,echoCancellation:true,noiseSuppression:true,autoGainControl:true},video:false});
    if(generation!==this.generation){stream.getTracks().forEach(track=>track.stop());return;}
    this.inputSource.disconnect();
    this.input.getTracks().forEach(track=>track.stop());
    this.input=stream;
    this.inputSource=this.context.createMediaStreamSource(stream);
    this.inputSource.connect(this.sendGain);
  }

  async setMicrophoneEnabled(enabled,deviceId='',capture) {
    const epoch=this.captureEpoch=(this.captureEpoch??0)+1;
    this.settings.microphone=false;this.applySettings();
    this.inputSource?.disconnect();this.input?.getTracks().forEach(track=>track.stop());this.input=null;
    if(!enabled||!this.context)return;
    const generation=this.generation;
    const stream=await(capture?capture(this.context):navigator.mediaDevices.getUserMedia({audio:{deviceId:deviceId?{exact:deviceId}:undefined,echoCancellation:true,noiseSuppression:true,autoGainControl:true},video:false}));
    if(epoch!==this.captureEpoch||generation!==this.generation){stream.getTracks().forEach(track=>track.stop());return;}
    this.input=stream;this.inputSource=this.context.createMediaStreamSource(stream);this.inputSource.connect(this.sendGain);
    this.settings.microphone=true;this.applySettings();
  }

  select(channel,room='main'){this.send({type:'select',channel,room});}
  createRoom(name){this.send({type:'create',name});}
  createPersonalRoom(name,pin){this.send({type:'createRoom',name,pin});}
  joinPersonalRoom(room,pin){this.send({type:'joinRoom',room,pin});}

  async testConnectionTone() {
    if(!this.context||!this.state||this.peers.size===0||this.settings.microphone||this.settings.testTone)return false;
    const context=this.context,generation=this.generation;
    const oscillator=context.createOscillator(),gain=context.createGain();
    oscillator.frequency.value=440;gain.gain.value=.035;oscillator.connect(gain);gain.connect(this.sendGain);
    this.settings.testTone=true;this.applySettings();oscillator.start();
    try{await new Promise(resolve=>{oscillator.onended=resolve;oscillator.stop(context.currentTime+2);});}
    finally{oscillator.disconnect();gain.disconnect();if(generation===this.generation){this.settings.testTone=false;this.applySettings();}}
    return true;
  }
  send(message){if(this.socket?.readyState===WebSocket.OPEN)this.socket.send(JSON.stringify(message));}
  signal(id,data){this.send({type:'signal',to:id,data});}

  async applyState(state) {
    if(!state.self?.id||!Array.isArray(state.peers)||state.peers.length>24||!Array.isArray(state.iceServers))throw new Error('state');
    clearTimeout(this.deadline);
    const previousIce=JSON.stringify(this.state?.iceServers??[]);
    this.state=state;
    // An authorized signaling session is usable even while its peers negotiate.
    this.applySettings();
    this.events.state?.(state);
    this.events.status?.('connected');
    const expected=new Set(state.peers.map(peer=>peer.id));
    for(const id of this.peers.keys())if(!expected.has(id))this.removePeer(id);
    for(const view of state.peers){
      let peer=this.peers.get(view.id);
      if(!peer)peer=await this.createPeer(view);
      peer.view=view;
      this.updatePeerVolume(peer);
      if(previousIce!==JSON.stringify(state.iceServers)){
        // Preserve immutable bundle policy when refreshing the ICE servers.
        peer.pc.setConfiguration({...peer.pc.getConfiguration(),iceServers:state.iceServers});
        if(peer.pc.remoteDescription)peer.pc.restartIce();
      }
    }
    this.applySettings();
    this.events.state?.(state);
    this.events.status?.('connected');
  }

  async createPeer(view) {
    const pc=new RTCPeerConnection({iceServers:this.state.iceServers,bundlePolicy:'max-bundle'});
    const peer={pc,view,candidates:[],makingOffer:false,settingAnswer:false,ignoreOffer:false,polite:!view.initiator,retries:0};
    this.peers.set(view.id,peer);
    pc.ontrack=event=>{
      if(this.peers.get(view.id)!==peer||!this.context)return;
      peer.source?.disconnect();peer.gain?.disconnect();
      const stream=event.streams[0]??new MediaStream([event.track]);
      peer.audio?.pause();
      // Use the browser's WebRTC playout sink, including while the overlay is hidden.
      peer.audio=new Audio();peer.audio.autoplay=true;peer.audio.srcObject=stream;
      peer.audio.volume=0;
      void peer.audio.play().catch(error=>{this.lastError={stage:'playback',name:error?.name??'Error'};});
      peer.source=this.context.createMediaStreamSource(stream);
      peer.gain=this.context.createGain();
      peer.gain.gain.value=0;
      peer.meter=this.context.createAnalyser();peer.meter.fftSize=256;
      peer.meterBytes=new Float32Array(256);
      peer.source.connect(peer.gain);peer.gain.connect(peer.meter);
      this.updatePeerVolume(peer);
    };
    pc.onicecandidate=event=>{if(event.candidate)this.signal(view.id,{type:'candidate',candidate:event.candidate.toJSON()});};
    pc.onnegotiationneeded=()=>{if(view.initiator||pc.remoteDescription)void this.negotiate(peer);};
    pc.onconnectionstatechange=()=>{
      if(pc.connectionState==='failed'&&peer.retries++<2){pc.restartIce();void this.negotiate(peer);}
      if(pc.connectionState==='connected')peer.retries=0;
    };
    for(const track of this.destination.stream.getAudioTracks()){
      const sender=pc.addTrack(track,this.destination.stream);
      const parameters=sender.getParameters();
      if(parameters.encodings?.length){
        parameters.encodings[0].maxBitrate=24000;
        try{await sender.setParameters(parameters);}catch(error){this.lastError={stage:'bitrate',name:error?.name??'Error'};}
      }
    }
    return peer;
  }

  async negotiate(peer) {
    if(peer.makingOffer||peer.pc.signalingState!=='stable'||!this.state||!this.peers.has(peer.view.id))return;
    try {
      peer.makingOffer=true;
      await peer.pc.setLocalDescription();
      this.signal(peer.view.id,peer.pc.localDescription.toJSON());
    } catch {this.events.error?.('connection-failed');}
    finally {peer.makingOffer=false;}
  }

  async receiveSignal(message) {
    const peer=this.peers.get(message.from);
    if(!peer)return;
    const data=message.data;
    if(data.type==='candidate'){
      if(peer.ignoreOffer)return;
      if(!peer.pc.remoteDescription){if(peer.candidates.length<64)peer.candidates.push(data.candidate);return;}
      await peer.pc.addIceCandidate(data.candidate);
      return;
    }
    if(data.type!=='offer'&&data.type!=='answer')return;
    const ready=!peer.makingOffer&&(peer.pc.signalingState==='stable'||peer.settingAnswer);
    peer.ignoreOffer=data.type==='offer'&&!ready&&!peer.polite;
    if(peer.ignoreOffer)return;
    peer.settingAnswer=data.type==='answer';
    try {await peer.pc.setRemoteDescription({type:data.type,sdp:data.sdp});}
    finally {peer.settingAnswer=false;}
    for(const candidate of peer.candidates.splice(0))await peer.pc.addIceCandidate(candidate);
    if(data.type==='offer'){
      await peer.pc.setLocalDescription();
      this.signal(peer.view.id,peer.pc.localDescription.toJSON());
    }
  }

  mute(name,muted){if(muted)this.muted.add(name);else this.muted.delete(name);for(const peer of this.peers.values())this.updatePeerVolume(peer);}
  updatePeerVolume(peer){
    const gain=this.muted.has(peer.view.id)?0:Math.max(0,Math.min(1,peer.view.gain));
    if(peer.gain&&this.context)peer.gain.gain.setTargetAtTime(gain,this.context.currentTime,.06);
    if(peer.audio)peer.audio.volume=this.settings.deafen?0:gain*this.settings.volume;
  }
  removePeer(id){const peer=this.peers.get(id);if(!peer)return;peer.pc.ontrack=null;peer.pc.onconnectionstatechange=null;peer.pc.onnegotiationneeded=null;peer.pc.close();if(peer.audio){peer.audio.pause();peer.audio.srcObject=null;}peer.source?.disconnect();peer.gain?.disconnect();peer.meter?.disconnect();this.peers.delete(id);}

  measure() {
    if(!this.analyser)return;
    this.analyser.getFloatTimeDomainData(this.meterBytes);
    let sum=0;for(const value of this.meterBytes)sum+=value*value;
    this.events.level?.(Math.sqrt(sum/this.meterBytes.length),this.transmitting);
  }

  async diagnostics() {
    const peers=[];
    for(const peer of this.peers.values()){
      const stats=await peer.pc.getStats();
      let bytesReceived=0,bytesSent=0,audioEnergy=0,sourceEnergy=0;
      for(const row of stats.values()){
        if(row.type==='inbound-rtp'&&row.kind==='audio'){bytesReceived+=row.bytesReceived??0;audioEnergy+=row.totalAudioEnergy??0;}
        if(row.type==='outbound-rtp'&&row.kind==='audio')bytesSent+=row.bytesSent??0;
        if(row.type==='media-source'&&row.kind==='audio')sourceEnergy+=row.totalAudioEnergy??0;
      }
      peers.push({id:peer.view.id,state:peer.pc.connectionState,bytesReceived,bytesSent,audioEnergy,sourceEnergy,
        outgoingEnabled:this.destination.stream.getAudioTracks()[0]?.enabled??false,
        incomingMuted:peer.pc.getReceivers().find(receiver=>receiver.track?.kind==='audio')?.track.muted??true,
        playback:peer.audio?{paused:peer.audio.paused,volume:peer.audio.volume,ready:peer.audio.readyState}:null});
    }
    return peers;
  }
}
