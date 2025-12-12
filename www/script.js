let autoRefresh = true;
let refreshInterval;
let timeSeriesData = [];
const maxDataPoints = 20;
let serverStartTime = Date.now();

// Initialize charts
function initCharts() {
    // Chart initialization logic placeholder
}

// Fetch stats from server (mock data for now)
async function fetchStats() {
    try {
        // Try to fetch from actual endpoint
        const response = await fetch('/api/stats');
        if (response.ok) {
            return await response.json();
        }
    } catch (e) {
        console.log('Using mock data');
    }

    // Mock data generator for demonstration
    const mock = {
        total_requests: Math.floor(Math.random() * 1000) + 500,
        status_200: Math.floor(Math.random() * 800) + 400,
        status_404: Math.floor(Math.random() * 150) + 50,
        status_500: Math.floor(Math.random() * 50) + 10,
        bytes_served: Math.floor(Math.random() * 1000000000),
        cache_hits: Math.floor(Math.random() * 600) + 300,
        cache_misses: Math.floor(Math.random() * 200) + 100
    };
    
    return mock;
}

// Update dashboard with new data
async function updateDashboard() {
    const stats = await fetchStats();
    
    // Update cards
    document.getElementById('totalRequests').textContent = stats.total_requests.toLocaleString();
    document.getElementById('requests200').textContent = stats.status_200.toLocaleString();
    document.getElementById('requests404').textContent = stats.status_404.toLocaleString();
    document.getElementById('requests500').textContent = stats.status_500.toLocaleString();
    
    // Calculate percentages
    const total = stats.total_requests || 1;
    document.getElementById('percent200').textContent = 
        `${((stats.status_200 / total) * 100).toFixed(1)}% of total`;
    document.getElementById('percent404').textContent = 
        `${((stats.status_404 / total) * 100).toFixed(1)}% of total`;
    document.getElementById('percent500').textContent = 
        `${((stats.status_500 / total) * 100).toFixed(1)}% of total`;
    
    // Bytes served
    const mb = (stats.bytes_served / (1024 * 1024)).toFixed(2);
    document.getElementById('bytesServed').textContent = `${mb} MB`;
    
    // Cache stats
    const cacheTotal = stats.cache_hits + stats.cache_misses || 1;
    const hitRate = ((stats.cache_hits / cacheTotal) * 100).toFixed(1);
    document.getElementById('cacheHitRate').textContent = `${hitRate}%`;
    document.getElementById('cacheHits').textContent = stats.cache_hits.toLocaleString();
    document.getElementById('cacheMisses').textContent = stats.cache_misses.toLocaleString();
    
    // Update time series
    const now = new Date().toLocaleTimeString();
    timeSeriesData.push({ time: now, requests: stats.total_requests });
    if (timeSeriesData.length > maxDataPoints) {
        timeSeriesData.shift();
    }
    
    // Update timestamp
    document.getElementById('lastUpdate').textContent = new Date().toLocaleString();
    
    // Update uptime
    const uptimeMs = Date.now() - serverStartTime;
    const hours = Math.floor(uptimeMs / 3600000);
    const minutes = Math.floor((uptimeMs % 3600000) / 60000);
    document.getElementById('uptime').textContent = `${hours}h ${minutes}m`;
    
    // Mock additional stats
    document.getElementById('avgResponseTime').textContent = 
        `${(Math.random() * 100 + 10).toFixed(0)}ms`;
    document.getElementById('reqPerSec').textContent = 
        `${(stats.total_requests / (uptimeMs / 1000)).toFixed(2)} req/s`;
}

function startAutoRefresh() {
    refreshInterval = setInterval(updateDashboard, 2000);
}

// Initialize on page load
window.addEventListener('load', () => {
    initCharts();
    updateDashboard();
    startAutoRefresh();
});