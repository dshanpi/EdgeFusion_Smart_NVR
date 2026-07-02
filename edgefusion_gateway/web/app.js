/* EdgeFusion NVR — Single Page App (Professional UI) */

const $ = (s) => document.querySelector(s);
const $$ = (s) => document.querySelectorAll(s);

/* ─── Toast ─── */
function toast(msg, ok = true) {
  const t = $('#toast');
  t.innerHTML = (ok ? '✓' : '✕') + ' ' + msg;
  t.className = 'toast ' + (ok ? 'ok' : 'err') + ' show';
  clearTimeout(t._tid);
  t._tid = setTimeout(() => t.classList.remove('show'), 2800);
}

/* ─── Formatting ─── */
function fmtTs(ts) {
  const d = new Date(ts * 1000);
  return d.toLocaleString('zh-CN', { hour12: false });
}
function fmtTime(ts) {
  const d = new Date(ts * 1000);
  return d.toLocaleTimeString('zh-CN', { hour12: false });
}
function fmtSize(b) {
  if (b == null) return '—';
  if (b < 1024) return b + ' B';
  if (b < 1048576) return (b / 1024).toFixed(1) + ' KB';
  if (b < 1073741824) return (b / 1048576).toFixed(1) + ' MB';
  return (b / 1073741824).toFixed(2) + ' GB';
}
/* parse timestamp out of recording filename rec_YYYYMMDD_HHMMSS_NN.mp4 */
function parseRecName(name) {
  const m = name.match(/rec_(\d{8})_(\d{6})_(\d+)\.mp4/i);
  if (!m) return null;
  const d = m[1], t = m[2];
  const ds = d.slice(0,4)+'-'+d.slice(4,6)+'-'+d.slice(6,8);
  const ts = t.slice(0,2)+':'+t.slice(2,4)+':'+t.slice(4,6);
  return { date: ds, time: ts, label: `${ds} ${ts}` };
}
function escHtml(s) {
  const d = document.createElement('div');
  d.textContent = s == null ? '' : String(s);
  return d.innerHTML;
}
function escAttr(s) {
  if (s == null) return '';
  return String(s).replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

/* ─── Navigation ─── */
const TITLES = {
  live: '实时预览', recordings: '录像回放',
  events: '事件告警', settings: '系统设置'
};
function navigate(page) {
  $$('.sidebar-nav a').forEach(a => a.classList.toggle('active', a.dataset.page === page));
  $('#topbar-title').textContent = TITLES[page] || page;
  switch (page) {
    case 'live': renderLive(); break;
    case 'recordings': renderRecordings(); break;
    case 'events': renderEvents(); break;
    case 'settings': renderSettings(); break;
  }
}
$$('.sidebar-nav a').forEach(a => {
  a.addEventListener('click', (e) => {
    e.preventDefault();
    window.location.hash = a.dataset.page;
    navigate(a.dataset.page);
  });
});
window.addEventListener('hashchange', () => navigate(window.location.hash.slice(1) || 'live'));

/* ─── Status Poll ─── */
let lastStatus = null;
let livePollTimer = null;

async function pollStatus() {
  try {
    const r = await fetch('/api/status');
    const s = await r.json();
    lastStatus = s;
    updateSystemChrome(s);
    if (typeof onStatusUpdate === 'function') {
      try { onStatusUpdate(s); } catch(e) { console.error('status render error', e); }
    }
  } catch(e) {
    /* network/parse error only — keep last good status, just flag disconnected */
    updateSystemChrome(null);
  }
}

function updateSystemChrome(s) {
  const online = s && s.stream && s.stream.connected;
  const rec = s && s.stream && s.stream.recording;
  const ai = s && s.cloud_ai;
  const aiOn = ai && ai.enabled;
  const aiRun = ai && ai.running;

  /* sidebar foot */
  setClass($('#sys-stream-dot'), online ? 'sys-dot on' : 'sys-dot off');
  $('#sys-stream-val').textContent = online
    ? `${s.stream.width}×${s.stream.height}@${s.stream.fps}`
    : '离线';
  setClass($('#sys-rec-dot'), 'sys-dot rec' + (rec ? ' live' : ''));
  $('#sys-rec-val').textContent = rec ? '录像中' : '空闲';
  setClass($('#sys-ai-dot'), aiRun ? 'sys-dot ai' : (aiOn ? 'sys-dot on' : 'sys-dot'));
  $('#sys-ai-val').textContent = aiRun ? '运行中' : (aiOn ? '待命' : '禁用');

  /* topbar pills */
  const ps = $('#pill-stream');
  ps.className = 'pill' + (online ? ' ok' : '');
  ps.querySelector('span:last-child').textContent = online ? 'LIVE' : 'NO SIGNAL';
  const pr = $('#pill-rec');
  pr.hidden = !rec;
}
function setClass(el, cls) { if (el) el.className = cls; }

/* clock */
function tickClock() {
  const d = new Date();
  const p = (n) => String(n).padStart(2, '0');
  $('#sidebar-clock').textContent = `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`;
}
setInterval(tickClock, 1000); tickClock();
setInterval(pollStatus, 4000); pollStatus();

/* hook for live page status-dependent refresh (set by renderLive) */
let onStatusUpdate = null;

/* ─────────────────────────────────────────────────────────
   Page: Live
   ───────────────────────────────────────────────────────── */
function renderLive() {
  onStatusUpdate = refreshLiveStatus;
  $('#content').innerHTML = `
    <div class="live-wrap">
      <div class="card" style="margin-bottom:0">
        <div class="card-head">
          <h3>通道 01 · CAM-V821</h3>
          <div class="card-tools">
            <span class="rec-dur" id="live-uptime">连接中…</span>
            <button class="btn btn-sm btn-ghost" onclick="toggleStream()" id="btn-stream">停止预览</button>
            <button class="btn btn-sm" onclick="snapshotNow()">
              <svg class="ic" viewBox="0 0 24 24"><path d="M4 7h3l2-2h6l2 2h3v12H4z" fill="none" stroke="currentColor" stroke-width="1.6"/><circle cx="12" cy="13" r="3.4" fill="none" stroke="currentColor" stroke-width="1.6"/></svg>
              抓拍
            </button>
          </div>
        </div>
        <div class="live-stage" id="live-stage">
          <img src="/stream" alt="Live MJPEG" id="live-img"
               onerror="imgFail()" onload="imgOk()">
          <div class="osd" id="live-osd"></div>
          <div class="no-signal" id="no-signal" style="display:none">
            <div class="ns-title">NO SIGNAL</div>
            <div class="ns-sub" id="ns-sub">正在尝试连接视频源…</div>
          </div>
        </div>
      </div>

      <div class="tiles" id="live-tiles"></div>
      <div id="snapshot-preview"></div>
    </div>
  `;
  refreshLiveStatus(lastStatus);
}

let streamStopped = false;
function toggleStream() {
  const img = $('#live-img');
  const btn = $('#btn-stream');
  streamStopped = !streamStopped;
  if (streamStopped) {
    img.src = '';
    img.style.display = 'none';
    btn.textContent = '恢复预览';
    $('#no-signal').style.display = 'flex';
    $('#ns-sub').textContent = '预览已停止';
  } else {
    img.src = '/stream?_=' + Date.now();
    img.style.display = 'block';
    btn.textContent = '停止预览';
  }
}
function imgFail() {
  const img = $('#live-img');
  if (streamStopped) return;
  img.style.display = 'none';
  $('#no-signal').style.display = 'flex';
  $('#ns-sub').textContent = '视频源未连接或拉流失败';
}
function imgOk() {
  $('#live-img').style.display = 'block';
  $('#no-signal').style.display = 'none';
}

function refreshLiveStatus(s) {
  if (!$('#live-tiles')) return; /* not on live page */
  s = s || lastStatus || {};
  const st = s.stream || {};
  const rs = s.rtsp_server || {};
  const ai = s.cloud_ai || {};

  /* OSD overlay */
  const osd = $('#live-osd');
  if (osd) {
    const online = st.connected;
    const now = new Date().toLocaleString('zh-CN', { hour12: false });
    osd.innerHTML = `
      <div class="osd-tl">
        <div class="osd-tag ${st.recording ? 'rec' : ''}">
          <span class="d"></span>${st.recording ? 'REC' : (online ? 'LIVE' : 'OFFLINE')}
        </div>
        <div class="osd-line"><span class="lab">CH</span> 01 · CAM-V821</div>
      </div>
      <div class="osd-tr">
        <div class="osd-line">${online ? `${st.width}×${st.height} · ${st.fps}fps` : ''}</div>
        <div class="osd-line">${st.bitrate_kbps ? st.bitrate_kbps + ' kbps' : ''}</div>
      </div>
      <div class="osd-bl">
        <div class="osd-line">${escHtml(now)}</div>
      </div>
      <div class="osd-br">
        <div class="osd-line">${rs.clients != null ? 'RTSP 客户端: ' + rs.clients : ''}</div>
        <div class="osd-line">${ai.enabled ? (ai.running ? 'AI: 分析中' : 'AI: 待命') : 'AI: 关闭'}</div>
      </div>
    `;
  }

  /* tiles */
  const tiles = [
    { label: '视频源', value: st.connected ? '在线' : '离线', cls: st.connected ? 'good' : 'bad', cardCls: st.connected ? 'ok' : 'bad', sub: st.connected ? 'RTSP 拉流' : '未连接' },
    { label: '分辨率', value: st.width ? `${st.width}×${st.height}` : '—', cls: '', cardCls: 'info', sub: st.fps ? st.fps + ' fps' : '' },
    { label: '录像', value: st.recording ? '录制中' : '空闲', cls: st.recording ? 'good' : '', cardCls: st.recording ? 'ok' : '', sub: st.cur_segment ? st.cur_segment : '等待关键帧' },
    { label: '码率', value: st.bitrate_kbps ? st.bitrate_kbps : '—', cls: '', cardCls: 'info', sub: 'kbps' },
    { label: 'RTSP 转发', value: rs.clients != null ? rs.clients : '—', cls: rs.clients > 0 ? 'info' : '', cardCls: rs.clients > 0 ? 'info' : '', sub: '客户端数' },
    { label: '云端 AI', value: ai.running ? '运行' : (ai.enabled ? '待命' : '禁用'),
      cls: ai.running ? 'good' : (ai.enabled ? 'warn' : ''), cardCls: ai.running ? 'ok' : (ai.enabled ? 'warn' : ''), sub: ai.enabled ? '已启用' : '本地模式' },
    { label: '事件总数', value: s.events_total != null ? s.events_total : '—', cls: '', cardCls: 'info', sub: '条记录' },
  ];
  $('#live-tiles').innerHTML = tiles.map(t => `
    <div class="tile ${t.cardCls}">
      <div class="t-label">${t.label}</div>
      <div class="t-value ${t.cls}">${t.value}</div>
      <div class="t-sub">${t.sub}</div>
    </div>`).join('');

  /* uptime-ish */
  const up = $('#live-uptime');
  if (up) up.textContent = st.connected ? '拉流活跃中' : '等待视频源…';
}

async function snapshotNow() {
  try {
    const r = await fetch('/api/snapshot');
    if (!r.ok) throw new Error('No frame');
    const blob = await r.blob();
    const url = URL.createObjectURL(blob);
    $('#snapshot-preview').innerHTML = `
      <div class="card">
        <div class="card-head">
          <h3>抓拍快照</h3>
          <div class="card-tools">
            <a class="btn btn-sm" href="${url}" download="snapshot_${Date.now()}.jpg">下载</a>
            <button class="btn btn-sm btn-ghost" onclick="document.getElementById('snapshot-preview').innerHTML=''">关闭</button>
          </div>
        </div>
        <div class="card-body" style="text-align:center">
          <img src="${url}" style="max-width:100%;border-radius:6px" onload="URL.revokeObjectURL('${url}')">
        </div>
      </div>`;
    toast('抓拍成功', true);
  } catch(e) {
    toast('抓拍失败：无可用帧', false);
  }
}

/* ─────────────────────────────────────────────────────────
   Page: Recordings
   ───────────────────────────────────────────────────────── */
async function renderRecordings() {
  onStatusUpdate = null;
  $('#content').innerHTML = `
    <div class="card" style="margin-bottom:16px">
      <div class="card-head">
        <h3>回放播放器</h3>
        <div class="card-tools">
          <button class="btn btn-sm btn-ghost" onclick="loadRecordings()"><svg class="ic" viewBox="0 0 24 24"><path d="M4 12a8 8 0 1 1 2.3 5.6M4 12V7M4 12h5" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round"/></svg>刷新</button>
        </div>
      </div>
      <div style="padding:14px 16px;background:#000;border-radius:0">
        <video id="player" class="rec-player" controls playsinline>
          您的浏览器不支持 video 标签。
        </video>
      </div>
      <div class="now-playing">
        <span class="np-label">当前播放:</span>
        <span class="np-name" id="now-playing">未选择录像</span>
        <span class="rec-dur" id="now-dur" style="margin-left:auto"></span>
      </div>
    </div>

    <div class="card">
      <div class="card-head">
        <h3>录像文件</h3>
        <span class="rec-dur" id="rec-summary"></span>
      </div>
      <div class="card-body" style="padding:0">
        <div class="tbl-wrap">
          <table class="tbl" id="rec-table">
            <thead><tr>
              <th style="width:36px">#</th>
              <th>开始时间</th>
              <th>文件名</th>
              <th>大小</th>
              <th style="width:120px;text-align:right">操作</th>
            </tr></thead>
            <tbody id="rec-list"><tr><td colspan="5" class="muted" style="padding:24px;text-align:center"><span class="spin"></span> 加载中…</td></tr></tbody>
          </table>
        </div>
      </div>
    </div>
  `;
  loadRecordings();
}

async function loadRecordings() {
  try {
    const r = await fetch('/api/recordings');
    const a = await r.json();
    const tbody = $('#rec-list');
    if (!a.length) {
      tbody.innerHTML = '<tr><td colspan="5"><div class="empty-state"><div class="es-icon">📭</div><div class="es-msg">暂无录像文件</div></div></td></tr>';
      $('#rec-summary').textContent = '';
      return;
    }
    /* sort newest first (by parsed time, fallback to name desc) */
    a.sort((x, y) => {
      const px = parseRecName(x.name), py = parseRecName(y.name);
      if (px && py) return py.label.localeCompare(px.label);
      return y.name.localeCompare(x.name);
    });
    const total = a.reduce((s, x) => s + (x.size || 0), 0);
    $('#rec-summary').textContent = `${a.length} 个文件 · 共 ${fmtSize(total)}`;

    tbody.innerHTML = a.map((x, i) => {
      const p = parseRecName(x.name);
      return `
        <tr>
          <td class="muted mono">${String(i + 1).padStart(2, '0')}</td>
          <td>${p ? `<div>${p.date}</div><div class="muted mono" style="font-size:11px">${p.time}</div>` : '<span class="muted">—</span>'}</td>
          <td class="mono" style="font-size:11.5px">${escHtml(x.name)}</td>
          <td class="mono">${fmtSize(x.size)}</td>
          <td style="text-align:right">
            <button class="btn btn-sm" onclick="playRecording('${escAttr(x.name)}')">播放</button>
            <a class="btn btn-sm btn-ghost" href="/rec/${encodeURIComponent(x.name)}" download>下载</a>
          </td>
        </tr>`;
    }).join('');
  } catch(e) {
    $('#rec-list').innerHTML = '<tr><td colspan="5"><div class="empty-state"><div class="es-icon">⚠</div><div class="es-msg">录像列表加载失败</div></div></td></tr>';
  }
}

function playRecording(name) {
  const v = $('#player');
  v.src = '/rec/' + encodeURIComponent(name);
  v.load();
  v.play().catch(() => {});
  $('#now-playing').textContent = name;
  const p = parseRecName(name);
  $('#now-dur').textContent = p ? p.label : '';
}

/* ─────────────────────────────────────────────────────────
   Page: Events
   ───────────────────────────────────────────────────────── */
let evPage = 0, evType = '', evLimit = 20;
async function renderEvents() {
  onStatusUpdate = null;
  $('#content').innerHTML = `
    <div class="card">
      <div class="card-head">
        <h3>事件告警列表</h3>
        <div class="card-tools">
          <button class="btn btn-sm btn-ghost" onclick="loadEvents()"><svg class="ic" viewBox="0 0 24 24"><path d="M4 12a8 8 0 1 1 2.3 5.6M4 12V7M4 12h5" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round"/></svg>刷新</button>
        </div>
      </div>
      <div class="card-body">
        <div class="filter-bar">
          <select id="ev-type-filter">
            <option value="">全部类型</option>
            <option value="person_detected">人员检测 (person_detected)</option>
            <option value="face_detected">人脸检测 (face_detected)</option>
            <option value="danger_alert">危险告警 (danger_alert)</option>
            <option value="none">无 (none)</option>
          </select>
          <button class="btn btn-sm" onclick="evPage=0;loadEvents()">筛选</button>
        </div>
        <div class="event-list" id="ev-list"><div class="empty-state"><span class="spin"></span> 加载中…</div></div>
        <div class="pagination" id="ev-pager"></div>
      </div>
    </div>
  `;
  evPage = 0;
  loadEvents();
}

async function loadEvents() {
  const type = $('#ev-type-filter')?.value || '';
  evType = type;
  const offset = evPage * evLimit;
  const list = $('#ev-list');
  list.innerHTML = '<div class="empty-state"><span class="spin"></span> 加载中…</div>';
  try {
    let url = `/api/events?limit=${evLimit}&offset=${offset}`;
    if (type) url += '&type=' + encodeURIComponent(type);
    const r = await fetch(url);
    const a = await r.json();
    if (!a.length) {
      list.innerHTML = `<div class="empty-state"><div class="es-icon">🔔</div><div class="es-msg">${evPage === 0 ? '暂无事件记录' : '没有更多事件'}</div></div>`;
      $('#ev-pager').innerHTML = evPage > 0 ? prevBtn() : '';
      return;
    }
    list.innerHTML = a.map(ev => {
      const conf = Math.round((ev.confidence || 0) * 100);
      return `
        <div class="event-card ${ev.event_type}">
          ${ev.snapshot
            ? `<img class="event-thumb" src="/snap/${encodeURIComponent(ev.snapshot)}" alt="snapshot" loading="lazy" onerror="this.outerHTML='<div class=\\'event-thumb-ph\\'>无图</div>'">`
            : `<div class="event-thumb-ph">无快照</div>`}
          <div class="event-body">
            <div class="event-head">
              <span class="ev-badge ${ev.event_type}">${ev.event_type}</span>
              <span class="ev-conf">置信度 ${conf}%<span class="bar"><i style="width:${conf}%"></i></span></span>
              <span class="ev-meta" style="margin:0">${fmtTs(ev.ts)} · #${ev.id}</span>
            </div>
            <div class="ev-desc">${escHtml(ev.description) || '<span class="muted">（无描述）</span>'}</div>
            <div class="ev-action">建议动作: <b>${escHtml(ev.suggested_action) || '—'}</b></div>
          </div>
        </div>`;
    }).join('');
    $('#ev-pager').innerHTML = `
      ${evPage > 0 ? `<button class="btn btn-sm" onclick="evPage--;loadEvents()">← 上一页</button>` : ''}
      <span class="pg-info">第 ${evPage + 1} 页</span>
      <button class="btn btn-sm" ${a.length < evLimit ? 'disabled' : ''} onclick="evPage++;loadEvents()">下一页 →</button>
    `;
  } catch(e) {
    list.innerHTML = '<div class="empty-state"><div class="es-icon">⚠</div><div class="es-msg">事件列表加载失败</div></div>';
  }
}
function prevBtn() {
  return `<div class="pagination"><button class="btn btn-sm" onclick="evPage--;loadEvents()">← 上一页</button></div>`;
}

/* ─────────────────────────────────────────────────────────
   Page: Settings
   ───────────────────────────────────────────────────────── */
async function renderSettings() {
  onStatusUpdate = null;
  $('#content').innerHTML = '<div class="empty-state"><span class="spin"></span> 加载配置…</div>';
  try {
    const r = await fetch('/api/config');
    const cfg = await r.json();
    $('#content').innerHTML = buildSettingsForm(cfg);
  } catch(e) {
    $('#content').innerHTML = '<div class="empty-state"><div class="es-icon">⚠</div><div class="es-msg">配置加载失败</div></div>';
  }
}

function toggleSel(name, on) {
  /* sync hidden select for boolean fields from switch */
  const sel = document.querySelector(`select[name="${name}"]`);
  if (sel) sel.value = on ? 'true' : 'false';
}

/* 开关即时保存：拨动即 POST 单字段，避免切页丢失未保存的开关状态。
 * 仅对轻量布尔字段用此路径（ai_enable/rtsp_server_enable/mqtt_enable）。 */
async function quickSaveBool(name, on) {
  try {
    const r = await fetch('/api/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({[name]: on})
    });
    const res = await r.json();
    if (res.ok) toast(on ? '已开启' : '已关闭', true);
    else toast('保存失败: ' + (res.error || '未知'), false);
  } catch(e) {
    toast('保存错误: ' + e.message, false);
  }
}

/* 云端 AI 当前实际状态提示（用于设置页卡片右上角） */
function aiStatusHint(c) {
  if (!c.ai_enable) return '已关闭';
  if (!c.anthropic_auth_token || c.anthropic_auth_token === '' ) return '已启用 · 待填 token';
  /* token 是掩码值（含 ****）或真实值都视为已配置 */
  return '已启用 · 运行中';
}

function buildSettingsForm(c) {
  /* 开关组件：整行(标签+拨片+状态文字)都可点击切换。
   * 用一个外层 <label> 包住 checkbox，点任意位置都切换。
   * onLabel=左侧标题；onState/offState=拨片右侧的状态文字。 */
  const sw = (name, val, onLabel, onState, offState) => `
    <div class="form-group">
      <label class="sw-title">${onLabel}</label>
      <label class="sw-row">
        <span class="switch"><input type="checkbox" ${val?'checked':''} onchange="toggleSel('${name}',this.checked);this.closest('.sw-row').querySelector('.sw-state').textContent=this.checked?'${onState}':'${offState}';this.closest('.sw-row').classList.toggle('off',!this.checked);quickSaveBool('${name}',this.checked)"><span class="slider"></span></span>
        <span class="sw-state ${val?'':'off'}">${val?onState:offState}</span>
      </label>
      <select name="${name}" hidden><option value="true" ${val?'selected':''}>true</option><option value="false" ${val?'':'selected'}>false</option></select>
    </div>`;

  return `
  <form id="cfg-form" onsubmit="saveConfig(event)">
    <div class="settings-grid">

      <div class="card">
        <div class="card-head"><h3>RTSP 视频源</h3></div>
        <div class="card-body">
          <div class="form-grid">
            <div class="form-group span-2"><label>主码流 URL</label><input name="rtsp_url" value="${escAttr(c.rtsp_url)}" placeholder="rtsp://192.168.1.28:8554/ch0"></div>
            <div class="form-group span-2"><label>子码流 URL (可选)</label><input name="rtsp_sub_url" value="${escAttr(c.rtsp_sub_url)}" placeholder="留空则用主码流降采样"></div>
            <div class="form-group"><label>传输协议</label>
              <select name="rtsp_transport">
                <option value="tcp" ${c.rtsp_transport==='tcp'?'selected':''}>TCP（推荐）</option>
                <option value="udp" ${c.rtsp_transport==='udp'?'selected':''}>UDP</option>
              </select></div>
            <div class="form-group"><label>拉流超时 (μs)</label><input type="number" name="rtsp_timeout_us" value="${c.rtsp_timeout_us}"></div>
            <div class="form-group"><label>断线重连 (秒)</label><input type="number" name="rtsp_reconnect_s" value="${c.rtsp_reconnect_s}"></div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-head"><h3>本地 RTSP 转发</h3></div>
        <div class="card-body">
          <div class="form-grid">
            ${sw('rtsp_server_enable', c.rtsp_server_enable, '启用转发', '已启用', '已禁用')}
            <div class="form-group"><label>监听端口</label><input type="number" name="rtsp_server_port" value="${c.rtsp_server_port}"></div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-head"><h3>录像存储</h3></div>
        <div class="card-body">
          <div class="form-grid">
            <div class="form-group span-2"><label>存储目录</label><input name="storage_dir" value="${escAttr(c.storage_dir)}"></div>
            <div class="form-group"><label>分段时长 (秒)</label><input type="number" name="segment_seconds" value="${c.segment_seconds}"></div>
            <div class="form-group"><label>最小剩余 (MB)</label><input type="number" name="storage_min_free_mb" value="${c.storage_min_free_mb}"></div>
            <div class="form-group"><label>文件名前缀</label><input name="segment_prefix" value="${escAttr(c.segment_prefix||'rec')}"></div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-head">
          <h3>云端 AI（OpenAI 兼容端点）</h3>
          <span class="rec-dur" id="ai-status-hint">${aiStatusHint(c)}</span>
        </div>
        <div class="card-body">
          <div class="form-grid">
            ${sw('ai_enable', c.ai_enable, '启用云端 AI', '已启用', '已禁用')}
            <div class="form-group"><label>AI 帧率 (fps)</label><input type="number" name="ai_fps" value="${c.ai_fps}"></div>
            <div class="form-group"><label>缩放宽度 (px)</label><input type="number" name="ai_frame_width" value="${c.ai_frame_width}"></div>
            <div class="form-group"><label>JPEG 质量</label><input type="number" name="ai_jpeg_quality" value="${c.ai_jpeg_quality}"></div>
            <div class="form-group"><label>HTTP 超时 (秒)</label><input type="number" name="ai_http_timeout_s" value="${c.ai_http_timeout_s}"></div>
            <div class="form-group span-2"><label>Base URL</label><input name="anthropic_base_url" value="${escAttr(c.anthropic_base_url)}"></div>
            <div class="form-group"><label>Auth Token</label><input type="password" name="anthropic_auth_token" value="${escAttr(c.anthropic_auth_token)}" placeholder="留空则纯本地模式"></div>
            <div class="form-group"><label>模型</label><input name="anthropic_model" value="${escAttr(c.anthropic_model)}"></div>
            <div class="form-group span-3"><label>Prompt</label><textarea name="ai_prompt" rows="2">${escHtml(c.ai_prompt)}</textarea></div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-head"><h3>报警联动 (GPIO)</h3></div>
        <div class="card-body">
          <div class="form-grid">
            <div class="form-group"><label>GPIO LED</label><input type="number" name="alarm_gpio_led" value="${c.alarm_gpio_led}"></div>
            <div class="form-group"><label>GPIO 蜂鸣器</label><input type="number" name="alarm_gpio_buzzer" value="${c.alarm_gpio_buzzer}"></div>
            <div class="form-group"><label>最小置信度</label><input type="number" step="0.01" name="alarm_min_confidence" value="${c.alarm_min_confidence}"></div>
            <div class="form-group"><label>动作时长 (秒)</label><input type="number" name="alarm_active_s" value="${c.alarm_active_s}"></div>
            <div class="form-group span-2"><label>触发类型 (逗号分隔)</label><input name="alarm_trigger_types" value="${escAttr(c.alarm_trigger_types)}"></div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-head"><h3>Web 服务</h3></div>
        <div class="card-body">
          <div class="form-grid">
            <div class="form-group"><label>绑定地址</label><input name="web_bind" value="${escAttr(c.web_bind)}"></div>
            <div class="form-group"><label>端口</label><input type="number" name="web_port" value="${c.web_port}"></div>
            <div class="form-group"><label>MJPEG 帧率</label><input type="number" name="web_mjpeg_fps" value="${c.web_mjpeg_fps}"></div>
            <div class="form-group"><label>Web 根目录</label><input name="web_root" value="${escAttr(c.web_root)}"></div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-head"><h3>MQTT 上报</h3></div>
        <div class="card-body">
          <div class="form-grid">
            ${sw('mqtt_enable', c.mqtt_enable, '启用 MQTT', '已启用', '已禁用')}
            <div class="form-group"><label>端口</label><input type="number" name="mqtt_port" value="${c.mqtt_port}"></div>
            <div class="form-group span-2"><label>Host</label><input name="mqtt_host" value="${escAttr(c.mqtt_host)}"></div>
            <div class="form-group"><label>Client ID</label><input name="mqtt_client_id" value="${escAttr(c.mqtt_client_id)}"></div>
            <div class="form-group"><label>Topic</label><input name="mqtt_topic" value="${escAttr(c.mqtt_topic)}"></div>
            <div class="form-group"><label>用户名</label><input name="mqtt_user" value="${escAttr(c.mqtt_user)}"></div>
            <div class="form-group"><label>密码</label><input type="password" name="mqtt_pass" value="${escAttr(c.mqtt_pass)}" placeholder="(不变则留空)"></div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-head"><h3>日志</h3></div>
        <div class="card-body">
          <div class="form-grid">
            <div class="form-group"><label>日志级别</label>
              <select name="log_level">
                <option value="DEBUG" ${c.log_level==='DEBUG'?'selected':''}>DEBUG</option>
                <option value="INFO" ${c.log_level==='INFO'?'selected':''}>INFO</option>
                <option value="WARN" ${c.log_level==='WARN'?'selected':''}>WARN</option>
                <option value="ERROR" ${c.log_level==='ERROR'?'selected':''}>ERROR</option>
              </select></div>
            <div class="form-group span-2"><label>日志文件</label><input name="log_file" value="${escAttr(c.log_file)}"></div>
          </div>
        </div>
      </div>

    </div>

    <div class="card" style="margin-bottom:0">
      <div class="sticky-save">
        <button type="submit" class="btn btn-primary">保存并应用配置</button>
        <button type="button" class="btn" onclick="renderSettings()">重新加载</button>
        <span class="ss-hint">保存后网关将热加载配置（部分项需重启进程生效）</span>
      </div>
    </div>
  </form>`;
}

async function saveConfig(e) {
  e.preventDefault();
  const fd = new FormData($('#cfg-form'));
  const obj = {};
  const strKeys = ['rtsp_url','rtsp_sub_url','rtsp_transport','storage_dir','segment_prefix',
    'anthropic_base_url','anthropic_auth_token','anthropic_model','ai_prompt',
    'alarm_trigger_types','web_bind','web_root','mqtt_host','mqtt_client_id','mqtt_topic',
    'mqtt_user','mqtt_pass','log_level','log_file'];
  fd.forEach((v, k) => {
    if (v === 'true') obj[k] = true;
    else if (v === 'false') obj[k] = false;
    else if (!isNaN(v) && v !== '' && !strKeys.includes(k)) obj[k] = Number(v);
    else obj[k] = v;
  });

  try {
    const r = await fetch('/api/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(obj)
    });
    const result = await r.json();
    if (result.ok) toast('配置已保存并应用', true);
    else toast('保存失败: ' + (result.error || '未知错误'), false);
  } catch(e) {
    toast('保存错误: ' + e.message, false);
  }
}

/* ─── Init ─── */
(function() {
  navigate(window.location.hash.slice(1) || 'live');
})();
