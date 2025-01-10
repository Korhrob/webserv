<?php
// Check if form was submitted via POST
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Retrieve email from POST request
    $email = trim($_POST['email']);

    // Validate email format
    if (filter_var($email, FILTER_VALIDATE_EMAIL)) {
        echo "<h2>Email is valid: $email</h2>";
    } else {
        echo "<h2 style='color: red;'>Invalid email address. Please try again.</h2>";
    }
} else {
    // Show an error if the script is accessed without form submission
    echo "<h2 style='color: red;'>Invalid request method.</h2>";
}
?>
