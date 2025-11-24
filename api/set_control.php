<?php
header('Content-Type: application/json');
// Simulate accepting control commands and return success
$data = json_decode(file_get_contents('php://input'), true);
$response = ['status' => 'ok', 'received' => $data];
echo json_encode($response);
?>