window.ParkingData = (() => {
  const config = window.PARKING_CONFIG;
  const mode = config.mode;
  const key = 'estacionamento-demo-v1';
  let memory;
  function initial() {
    return {status:{vagas:Array.from({length:4},(_,i)=>({numero:i+1,ocupada:false,entradaAtual:null})),ultimaAtualizacao:null},historico:[]};
  }
  function normalize(data) {
    const vagas=data?.status?.vagas;
    if(!Array.isArray(vagas)||vagas.length!==4||new Set(vagas.map(v=>v.numero)).size!==4||vagas.some(v=>!Number.isInteger(v.numero)||v.numero<1||v.numero>4||typeof v.ocupada!=='boolean'))throw Error('Aguardando dados válidos das quatro vagas.');
    if(data.status.ultimaAtualizacao&&!Number.isFinite(Date.parse(data.status.ultimaAtualizacao)))throw Error('Horário inválido.');
    vagas.sort((a,b)=>a.numero-b.numero);
    const ocupadas=vagas.filter(v=>v.ocupada).length;
    Object.assign(data.status,{total:4,ocupadas,livres:4-ocupadas,taxaOcupacao:ocupadas*25});
    data.historico=(Array.isArray(data.historico)?data.historico:[]).filter(e=>Number.isInteger(e.vaga)&&e.vaga>=1&&e.vaga<=4&&['ENTRADA','SAIDA'].includes(e.tipo)).slice(0,100);
    return data;
  }
  async function json(url,options={}) {
    const response=await fetch(url,{...options,cache:'no-store',signal:AbortSignal.timeout(5000)});
    if(!response.ok)throw Error(`Falha na leitura (${response.status}). Verifique configuração e permissões.`);
    return response.json();
  }
  function fields(f={}) {
    return Object.fromEntries(Object.entries(f).map(([k,v])=>[k,v.nullValue!==undefined?null:v.integerValue!==undefined?Number(v.integerValue):v.booleanValue??v.stringValue??v.timestampValue??v.doubleValue]));
  }
  async function read() {
    if(mode==='demo') {
      try{memory=normalize(JSON.parse(localStorage.getItem(key))||memory||initial());}catch{memory=memory||initial();}
      return normalize(structuredClone(memory));
    }
    if(mode==='esp32') {
      const base=config.esp32Url.replace(/\/$/,'');
      const [status,historico]=await Promise.all([json(`${base}/api/vagas`),json(`${base}/api/historico`)]);
      return normalize({status,historico});
    }
    if(mode!=='firebase')throw Error('Modo inválido em config.js.');
    if(!config.firebaseProjectId)throw Error('Preencha firebaseProjectId em config.js.');
    const base=`https://firestore.googleapis.com/v1/projects/${encodeURIComponent(config.firebaseProjectId)}/databases/(default)/documents`;
    const [vagas,meta,eventos]=await Promise.all([
      json(`${base}/vagas?pageSize=4`),json(`${base}/metadata/status`),
      json(`${base}:runQuery`,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({structuredQuery:{from:[{collectionId:'eventos'}],orderBy:[{field:{fieldPath:'dataHora'},direction:'DESCENDING'}],limit:100}})})
    ]);
    return normalize({status:{vagas:(vagas.documents||[]).map(d=>fields(d.fields)),ultimaAtualizacao:fields(meta.fields).ultimaAtualizacao},historico:eventos.filter(e=>e.document).map(e=>fields(e.document.fields))});
  }
  async function update({vaga,ocupada}) {
    if(mode!=='demo')throw Error('Somente a ESP32 deve alterar dados reais.');
    const data=await read();
    const item=data.status.vagas.find(v=>v.numero===vaga);
    if(!item||typeof ocupada!=='boolean')throw Error('Vaga inválida.');
    if(item.ocupada===ocupada)return data;
    const now=new Date().toISOString();
    data.historico.unshift({vaga,tipo:ocupada?'ENTRADA':'SAIDA',dataHora:now,entrada:ocupada?now:item.entradaAtual,saida:ocupada?null:now});
    Object.assign(item,{ocupada,entradaAtual:ocupada?now:null,ultimaAtualizacao:now});
    data.status.ultimaAtualizacao=now;
    memory=normalize(data);
    try{localStorage.setItem(key,JSON.stringify(memory));}catch{document.getElementById('database-mode').textContent='Dados temporários: armazenamento indisponível';}
    return structuredClone(memory);
  }
  return {mode,read,update};
})();
