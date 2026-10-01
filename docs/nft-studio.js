(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const key = 'ksmb-nft-drafts-v1';
  const fields = ['title', 'creator', 'description', 'network', 'style', 'accent', 'background', 'variation'];
  const escape = value => String(value).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&apos;'}[c]));
  const tell = message => { $('status').textContent = message; };
  function validate(value) {
    if (!value || typeof value !== 'object') throw Error('Invalid artwork record.');
    for (const [field, max] of [['title',36],['creator',28],['description',500]]) {
      if (typeof value[field] !== 'string' || value[field].length > max) throw Error('Invalid artwork text.');
    }
    if (!value.title.trim() || !['ethereum','bitcoin'].includes(value.network) || !['orbit','grid','gem'].includes(value.style)) throw Error('Invalid artwork settings.');
    if (![value.accent,value.background].every(c => /^#[0-9a-f]{6}$/i.test(c))) throw Error('Invalid artwork colors.');
    const variation = Number(value.variation);
    if (!Number.isInteger(variation) || variation < 1 || variation > 20) throw Error('Invalid artwork variation.');
    return {title:value.title,creator:value.creator,description:value.description,network:value.network,style:value.style,accent:value.accent,background:value.background,variation,featured:value.featured === true};
  }
  function design() { return validate(Object.fromEntries(fields.map(f => [f,$(f).value]))); }
  function svg(d) {
    let shapes = '';
    const angle = d.variation * 9;
    if (d.style === 'orbit') {
      for (let n=0;n<7;n++) shapes += `<ellipse cx="400" cy="360" rx="${110+n*27}" ry="${65+n*14}" transform="rotate(${angle+n*22} 400 360)" fill="none" stroke="${d.accent}" stroke-opacity="${.25+n*.09}" stroke-width="3"/>`;
      shapes += `<circle cx="400" cy="360" r="85" fill="url(#glow)"/>`;
    } else if (d.style === 'grid') {
      for(let n=0;n<12;n++) shapes += `<path d="M ${40+n*65} 80 L ${760-n*35} 650 M 40 ${80+n*48} H 760" stroke="${d.accent}" stroke-opacity=".3" fill="none"/>`;
      shapes += `<rect x="250" y="210" width="300" height="300" rx="35" transform="rotate(${angle} 400 360)" fill="url(#glow)" stroke="${d.accent}" stroke-width="3"/>`;
    } else {
      for(let n=0;n<6;n++) shapes += `<path d="M400 110 L610 360 L400 610 L190 360 Z" transform="rotate(${angle+n*30} 400 360) scale(1)" fill="${d.accent}" fill-opacity=".07" stroke="${d.accent}" stroke-opacity=".7" stroke-width="2"/>`;
    }
    return `<svg xmlns="http://www.w3.org/2000/svg" width="800" height="800" viewBox="0 0 800 800"><defs><radialGradient id="glow"><stop stop-color="${d.accent}"/><stop offset="1" stop-color="${d.background}"/></radialGradient></defs><rect width="800" height="800" fill="${d.background}"/><rect x="24" y="24" width="752" height="752" rx="28" fill="none" stroke="${d.accent}" stroke-opacity=".4"/>${shapes}<text x="60" y="78" fill="${d.accent}" font-family="sans-serif" font-size="15" letter-spacing="4">KSMB / CREATOR EDITION</text><text x="60" y="698" fill="#ffffff" font-family="sans-serif" font-size="${d.title.length>24?25:34}">${escape(d.title)}</text><text x="60" y="738" fill="${d.accent}" font-family="sans-serif" font-size="20">${escape(d.creator)}</text></svg>`;
  }
  const src = d => 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(svg(d));
  function download(content, name, type) {
    const url = URL.createObjectURL(new Blob([content], {type}));
    const a = document.createElement('a'); a.href=url; a.download=name; a.click();
    setTimeout(() => URL.revokeObjectURL(url),10000);
  }
  let records = [];
  let storageReadable = true;
  try {
    const saved = JSON.parse(localStorage.getItem(key) || '[]');
    if (!Array.isArray(saved) || saved.length > 100) throw Error('Invalid saved gallery.');
    records = saved.map(validate);
  } catch { storageReadable = false; tell('Saved gallery could not be read. Export or recover browser data before saving; storage will not be overwritten.'); }
  function save(next) {
    if (!storageReadable) { tell('Saving is blocked because the existing gallery could not be read.'); return false; }
    try { localStorage.setItem(key,JSON.stringify(next)); records=next; render(); return true; }
    catch { tell('Browser storage unavailable or full. Download your artwork or export a backup.'); return false; }
  }
  function action(label, callback) { const b=document.createElement('button'); b.type='button'; b.className='secondary'; b.textContent=label; b.addEventListener('click',callback); return b; }
  function render() {
    $('items').replaceChildren(); $('count').textContent=String(records.length);
    if(!records.length) { const e=document.createElement('p');e.className='empty';e.textContent='Your gallery starts with your first creation. Save a draft above.';$('items').append(e); }
    records.forEach((d,i) => {
      const card=document.createElement('article');card.className='panel art-card'+(d.featured?' featured':'');
      const img=document.createElement('img'); img.src=src(d);img.alt=d.title;img.width=800;img.height=800;
      const title=document.createElement('h3'); title.textContent=d.title;
      const caption=document.createElement('p'); caption.textContent=(d.featured?'Featured · ':'')+d.network.toUpperCase()+' draft · Not minted';
      const desc=document.createElement('p');desc.textContent=d.description;
      const buttons=document.createElement('div');buttons.className='actions';
      buttons.append(action('Edit',()=>{fields.forEach(f=>{$(f).value=d[f];});preview();$('title').focus();}),action('SVG',()=>download(svg(d),'artwork.svg','image/svg+xml')),action('Metadata',()=>download(JSON.stringify({name:d.title,description:d.description,image:'artwork.svg',attributes:[{trait_type:'Creator',value:d.creator},{trait_type:'Style',value:d.style}]},null,2),'metadata-draft.json','application/json')),action(d.featured?'Unfeature':'Feature',()=>save(records.map((r,n)=>({...r,featured:n===i?!r.featured:false})))),action('Remove',()=>{if(confirm('Remove this local draft? Export a backup first if you want to keep it.'))save(records.filter((_,n)=>n!==i));}));
      card.append(img,title,caption,desc,buttons);$('items').append(card);
    });
  }
  function preview(){try{const d=design();$('preview').src=src(d);$('networkLabel').textContent=d.network.toUpperCase()+' DRAFT';}catch{/* Keep the last preview while required text is empty. */}}
  $('designer').addEventListener('input',preview);
  $('designer').addEventListener('submit',event=>{event.preventDefault();try{if(records.length>=100)throw Error('Gallery limit: 100 drafts. Export a backup and remove older drafts.');if(save([...records,design()]))tell('Draft saved in this browser. It has not been minted.');}catch(e){tell(e.message);}});
  $('download').addEventListener('click',()=>{try{download(svg(design()),'artwork.svg','image/svg+xml');}catch(e){tell(e.message);}});
  $('backup').addEventListener('click',()=>download(JSON.stringify({version:1,drafts:records},null,2),'ksmb-gallery-backup.json','application/json'));
  $('import').addEventListener('change',async event=>{
    try{const file=event.target.files[0];if(!file)return;if(file.size>1000000)throw Error('Backup must be smaller than 1 MB.');const data=JSON.parse(await file.text());if(data.version!==1||!Array.isArray(data.drafts)||data.drafts.length+records.length>100)throw Error('Invalid backup or gallery limit exceeded.');const incoming=data.drafts.map(validate).map(d=>({...d,featured:false}));if(save([...records,...incoming]))tell('Backup imported as local artwork drafts.');}catch(e){tell(e.message);}finally{event.target.value='';}
  });
  render();preview();
})();
