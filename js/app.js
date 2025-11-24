// Sidebar collapse/expand logic
const sidebar = document.getElementById('sidebar');
const sidebarToggle = document.getElementById('sidebarToggle');

function expandSidebar() {
    sidebar.classList.add('expanded');
    sidebar.classList.remove('collapsed');
}
function collapseSidebar() {
    sidebar.classList.add('collapsed');
    sidebar.classList.remove('expanded');
}

// Mouse events for hover expand (desktop only)
function isDesktop() {
    return window.innerWidth > 700;
}
function setSidebarListeners() {
    if (isDesktop()) {
        sidebar.addEventListener('mouseenter', expandSidebar);
        sidebar.addEventListener('mouseleave', collapseSidebar);
    } else {
        sidebar.removeEventListener('mouseenter', expandSidebar);
        sidebar.removeEventListener('mouseleave', collapseSidebar);
    }
}
window.addEventListener('resize', setSidebarListeners);
setSidebarListeners();

// Click toggle for sticky open/close
sidebarToggle.addEventListener('click', () => {
    if (sidebar.classList.contains('collapsed')) {
        expandSidebar();
    } else {
        collapseSidebar();
    }
});

// Start collapsed by default
collapseSidebar();

// Page navigation logic
document.querySelectorAll('.nav-item').forEach(item => {
    item.addEventListener('click', () => {
        document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
        item.classList.add('active');
        let page = item.getAttribute('data-page');
        document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
        document.getElementById(page).classList.add('active');
    });
});

// Responsive sidebar toggle for mobile
const menuToggle = document.getElementById('menuToggle');
if (menuToggle) {
    menuToggle.addEventListener('click', () => {
        if (sidebar.classList.contains('collapsed')) {
            expandSidebar();
        } else {
            collapseSidebar();
        }
    });
}

// Demo UI updates (simulate API)
window.addEventListener('DOMContentLoaded', () => {
    // Simulate environmental data
    document.getElementById('mainTemp').textContent = "23°C";
    document.getElementById('tempValue').textContent = "22.4°C";
    document.getElementById('humidityValue').textContent = "67%";
    document.getElementById('ecValue').textContent = "1.8";
    document.getElementById('phValue').textContent = "6.2";

    // Progress bars
    document.getElementById('tempProgress').style.width = "75%";
    document.getElementById('humidityProgress').style.width = "67%";
    document.getElementById('ecProgress').style.width = "60%";
    document.getElementById('phProgress').style.width = "82%";

    // Misting system
    document.getElementById('mistStatus').textContent = "ON";
    document.getElementById('lastCycle').textContent = "45s";
    document.getElementById('nextCycle').textContent = "12:45";
    document.getElementById('mistProgress').style.width = "60%";

    // Nutrient slider
    const nutrientSlider = document.getElementById('nutrientSlider');
    const nutrientValue = document.getElementById('nutrientValue');
    nutrientSlider.addEventListener('input', e => {
        nutrientValue.textContent = e.target.value;
    });
    nutrientValue.textContent = nutrientSlider.value;
});