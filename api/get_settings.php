<?php
header('Content-Type: application/json');
// Simulated settings data
echo json_encode([
    'profile' => [
        'name' => 'John Smith',
        'role' => 'Hydroponic Specialist',
        'email' => 'john.smith@aerogrow.com',
        'crop' => 'Lettuce',
        'temp_unit' => 'celsius'
    ],
    'preferences' => [
        'push_notifications' => true,
        'auto_dosing' => true,
        'data_logging' => true
    ],
    'maintenance' => [
        'cpu' => 23,
        'memory' => 45,
        'storage' => 67,
        'sensors' => ['last' => 'Oct 20', 'next' => 'Nov 20'],
        'filter' => ['due' => 'Nov 5', 'days' => 3],
        'pump' => ['overdue' => 'Oct 30']
    ]
]);
?>