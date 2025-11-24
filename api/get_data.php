<?php
header('Content-Type: application/json');
// Simulated sensor and system data
echo json_encode([
    'temperature' => 22.4,
    'humidity' => 67,
    'ec' => 1.8,
    'ph' => 6.2,
    'water_level' => 25,
    'nutrient_level' => 78,
    'mix_level' => 92,
    'misting_status' => true,
    'last_cycle' => '45s',
    'next_cycle' => '12:45',
    'alerts' => [
        ['type' => 'error', 'message' => 'Low Water Supply: Water tank level is at 25%. Please refill to continue operations.'],
        ['type' => 'warning', 'message' => 'pH Level Notice: Current pH is 6.2 – within acceptable range (6.0-6.5) for lettuce growth.'],
        ['type' => 'success', 'message' => 'System Health Check: All pumps and sensors are functioning normally. Last check: 2 minutes ago.']
    ]
]);
?>