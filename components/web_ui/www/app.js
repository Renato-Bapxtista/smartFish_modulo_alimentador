async function fetchStatus(){
  try{
    const r=await fetch('/status');
    if(!r.ok) throw new Error('HTTP '+r.status);
    const json=await r.json();
    // populate tiles
    document.getElementById('temp').innerText = (json.temperature||24.5) + ' °C';
    document.getElementById('ph').innerText = (json.ph||7.25);
    document.getElementById('next-feed').innerText = (json.next_feed||'16:00 — 500g');
    const hist=document.getElementById('history'); hist.innerHTML='';
    (json.history||[]).slice(0,10).forEach(it=>{
      const li=document.createElement('li'); li.innerText = `${it.time} — ${it.action}`; hist.appendChild(li);
    });
  }catch(e){
    document.getElementById('temp').innerText='-- °C';
    document.getElementById('ph').innerText='--';
    document.getElementById('next-feed').innerText='--';
  }
}

document.getElementById('btn-refresh').addEventListener('click',fetchStatus);
document.getElementById('btn-feed-now').addEventListener('click',async ()=>{
  const amount = 1;
  try{
    const r = await fetch('/feed',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({amount})});
    if(!r.ok) throw new Error('fail');
    await fetchStatus();
    alert('Comando enviado');
  }catch(e){alert('Erro ao enviar comando');}
});

// initial
fetchStatus();
