// Initialize Firebase
try {
    firebase.initializeApp(firebaseConfig);
    const database = firebase.database();
    console.log("Firebase initialized successfully");

    const luxRef = database.ref('luminance');
    let devicesData = {};
    let sortBy = 'timestamp';

    // UI state
    const MAX_POINTS = 120;
    let devicesHistory = loadHistory();
    function loadHistory() { try { const raw = localStorage.getItem('luxHistory'); return raw ? JSON.parse(raw) : {}; } catch { return {}; } }
    function saveHistory() { try { localStorage.setItem('luxHistory', JSON.stringify(devicesHistory)); } catch {} }

    // Helpers
    function toMillis(ts) { if (!ts) return Date.now(); const n = Number(ts); if (isNaN(n)) return Date.now(); return n < 1e12 ? n * 1000 : n; }
    function getDeviceStatus(timestamp) {
        const diff = Date.now() - toMillis(timestamp);
        if (diff < 10000) return 'online';
        if (diff < 60000) return 'stale';
        return 'offline';
    }
    function getTimeAgo(timestamp) {
        const diff = Math.floor((Date.now() - toMillis(timestamp)) / 1000);
        if (diff < 60) return `${diff}s ago`;
        if (diff < 3600) return `${Math.floor(diff/60)}m ago`;
        if (diff < 86400) return `${Math.floor(diff/3600)}h ago`;
        return `${Math.floor(diff/86400)}d ago`;
    }
    function drawSparkline(canvas, points) {
        if (!canvas || !points || points.length < 2) return;
        const ctx = canvas.getContext('2d');
        const rect = canvas.getBoundingClientRect();
        canvas.width = Math.floor(rect.width);
        canvas.height = Math.floor(rect.height);
        const w = canvas.width, h = canvas.height;
        ctx.clearRect(0, 0, w, h);
        const xs = points.map(p => p.t);
        const ys = points.map(p => Number(p.lux)).filter(v => !isNaN(v));
        if (ys.length === 0) return;
        const minX = Math.min(...xs), maxX = Math.max(...xs);
        const minY = Math.min(...ys), maxY = Math.max(...ys);
        const dx = Math.max(1, maxX - minX);
        const dy = Math.max(0.0001, maxY - minY);
        ctx.strokeStyle = '#e0e6ed'; ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(0, h - 1); ctx.lineTo(w, h - 1); ctx.stroke();
        ctx.strokeStyle = '#3498db'; ctx.lineWidth = 2; ctx.beginPath();
        points.forEach((p, i) => {
            const x = ((p.t - minX) / dx) * (w - 4) + 2;
            const y = h - (((Number(p.lux) - minY) / dy) * (h - 4) + 2);
            if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        });
        ctx.stroke();
    }
    // Escapar HTML para alias seguro en UI
    function escapeHtml(s) {
        return String(s).replace(/[&<>"']/g, m => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[m]));
    }

    function renderDevices() {
        const container = document.getElementById('devicesContainer');
        if (!container) return;

        let list = Object.keys(devicesData).map(key => ({ key, ...devicesData[key] }));

        // Sort
        list.sort((a, b) => {
            if (sortBy === 'timestamp') return toMillis(b.timestamp) - toMillis(a.timestamp);
            if (sortBy === 'lux') return Number(b.lux) - Number(a.lux);
            if (sortBy === 'mac') return (a.mac || a.key).localeCompare(b.mac || b.key);
            return 0;
        });

        container.innerHTML = '';

        if (list.length === 0) {
            const empty = document.createElement('div');
            empty.className = 'device-card';
            empty.innerHTML = `<div class="device-header"><div class="device-mac">No devices available</div></div>`;
            container.appendChild(empty);
        }

        let activeCount = 0;

        list.forEach(device => {
            const mac = device.mac || device.key;
            const alias = (typeof device.alias === 'string' && device.alias.trim()) ? device.alias.trim() : '';
            const displayName = alias ? (`<strong class="device-alias">${escapeHtml(alias)}</strong> <span class="device-mac-code">(${mac})</span>`) : mac;

            const lux = Number(device.lux);
            const timestamp = device.timestamp;
            const status = getDeviceStatus(timestamp);
            const timeAgo = getTimeAgo(timestamp);
            if (status !== 'offline') activeCount++;

            // Build location HTML if available
            let locationHtml = '';
            if (device.geo) {
                const parts = [];
                if (device.geo.country) parts.push(escapeHtml(device.geo.country));
                if (device.geo.region) parts.push(escapeHtml(device.geo.region));
                if (device.geo.city) parts.push(escapeHtml(device.geo.city));
                if (parts.length) {
                    locationHtml = `<div class="device-location">${parts.join(' / ')}</div>`;
                }
                // If lat/lng are later added, UI can display map link; currently we avoid showing coordinates.
            }

            const card = document.createElement('div');
            card.className = 'device-card';
            card.innerHTML = `
                <div class="device-header">
                    <div class="device-mac">${displayName}</div>
                    <div class="device-status ${status}"></div>
                </div>
                <div class="device-lux">${isNaN(lux) ? 'N/A' : lux.toFixed(2)}</div>
                <div class="device-timestamp">${new Date(toMillis(timestamp)).toLocaleString()}</div>
                ${locationHtml}
                <div class="device-trend">
                    <span>Status: ${status}</span>
                    <span>${timeAgo}</span>
                </div>
                <div class="device-sparkline"><canvas></canvas></div>
            `;
            const canvas = card.querySelector('canvas');
            const series = (devicesHistory[mac] || []).slice(-MAX_POINTS);
            drawSparkline(canvas, series);

            card.title = 'Click to copy MAC';
            card.addEventListener('click', () => navigator.clipboard?.writeText(mac).catch(()=>{}));

            container.appendChild(card);
        });

        // Active devices = solo online + stale
        document.getElementById('deviceCount').textContent = activeCount;
    }

    function updateStats() {
        const devices = Object.values(devicesData);
        // Active devices (excluye offline) también aquí para mantenerlo correcto en refrescos temporales
        const activeCount = devices.filter(d => getDeviceStatus(d.timestamp) !== 'offline').length;
        document.getElementById('deviceCount').textContent = activeCount;

        if (devices.length === 0) {
            document.getElementById('luxValue').textContent = 'No data';
            document.getElementById('timestamp').textContent = 'N/A';
            // Last Update shows relative time; no data => empty
            document.getElementById('lastUpdateTime').textContent = 'Never';
            const labelEl = document.getElementById('lastUpdateLabel');
            if (labelEl) labelEl.textContent = '';
            return;
        }
        const latest = devices.reduce((latest, device) => (toMillis(device.timestamp) > toMillis(latest.timestamp) ? device : latest));
        document.getElementById('luxValue').textContent = Number(latest.lux).toFixed(2);
        // exact timestamp shown under Current Lux
        document.getElementById('timestamp').textContent = new Date(toMillis(latest.timestamp)).toLocaleString();
        // Last Update: show relative elapsed time since latest.timestamp
        const lastTimeEl = document.getElementById('lastUpdateTime');
        if (lastTimeEl) lastTimeEl.textContent = getTimeAgo(latest.timestamp);
        // clear auxiliary label
        const labelEl = document.getElementById('lastUpdateLabel');
        if (labelEl) labelEl.textContent = '';
    }

    // Data subscription
    luxRef.on('value', (snapshot) => {
        const raw = snapshot.val();
        const debugEl = document.getElementById('debugRaw');
        if (debugEl) debugEl.textContent = raw ? JSON.stringify(raw, null, 2) : 'null';

        if (!snapshot.exists()) {
            devicesData = {};
            renderDevices(); updateStats();
            return;
        }

        devicesData = {};
        if (snapshot.hasChildren()) {
            snapshot.forEach(child => {
                const key = child.key;
                const v = child.val();

                // ...existing check: accept object nodes even if lux is missing (we still want geo)...
                if (v && typeof v === 'object') {
                    const lux = (v.lux !== undefined) ? Number(v.lux) : NaN;
                    const tsMs = v.timestamp !== undefined ? toMillis(v.timestamp) : Date.now();
                    const alias = (typeof v.alias === 'string' && v.alias.trim()) ? v.alias.trim() : undefined;

                    // Explicitly read geo child if present
                    let geo = undefined;
                    if (child.hasChild && child.hasChild('geo')) {
                        const gv = child.child('geo').val();
                        if (gv && typeof gv === 'object') geo = gv;
                    } else if (v.geo && typeof v.geo === 'object') {
                        // fallback: geo already inside v
                        geo = v.geo;
                    }

                    // Build device entry including geo (if any)
                    devicesData[key] = { ...v, lux, alias, timestamp: v.timestamp, geo };

                    const mac = v.mac || key;
                    if (!devicesHistory[mac]) devicesHistory[mac] = [];
                    // only push valid numeric lux into history
                    if (!isNaN(lux)) {
                        devicesHistory[mac].push({ t: tsMs, lux });
                        if (devicesHistory[mac].length > MAX_POINTS) {
                            devicesHistory[mac].splice(0, devicesHistory[mac].length - MAX_POINTS);
                        }
                    }
                }
            });
            saveHistory();
        }

        renderDevices(); updateStats();
    }, (error) => {
        console.error('Error reading from database:', error);
        document.getElementById('luxValue').textContent = 'Error';
    });

    // Controls
    const sortSelect = document.getElementById('sortSelect');
    sortSelect && sortSelect.addEventListener('change', (e) => {
        sortBy = e.target.value; renderDevices();
    });

    const refreshBtn = document.getElementById('refreshBtn');
    refreshBtn && refreshBtn.addEventListener('click', () => {
        renderDevices(); updateStats();
    });

    const debugToggle = document.getElementById('debugToggle');
    debugToggle && debugToggle.addEventListener('click', (e) => {
        const debugContent = document.getElementById('debugRaw');
        debugContent.classList.toggle('show');
        e.target.textContent = debugContent.classList.contains('show') ? 'Hide Debug Data' : 'Show Debug Data';
    });

    // Periodic refresh for time-ago
    setInterval(() => { updateStats(); renderDevices(); }, 30000);

} catch (error) {
    console.error('Error initializing Firebase:', error);
    document.getElementById('luxValue').textContent = 'Init Error';
}
