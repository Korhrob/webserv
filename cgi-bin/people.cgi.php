<?php

$filename = 'people.txt';

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['first_name'], $_POST['last_name'])) {
    $firstName = trim($_POST['first_name']);
    $lastName = trim($_POST['last_name']);

    if (!empty($firstName) && !empty($lastName)) {
        $file = fopen($filename, 'a');
        if ($file) {
            fwrite($file, $firstName . ' ' . $lastName . "\n");
            fclose($file);
            echo "Person added successfully!";
        } else {
            echo "Error saving data!";
        }
    } else {
        echo "First and Last names are required.";
    }
}

if ($_SERVER['REQUEST_METHOD'] === 'GET' && isset($_GET['search_name'], $_GET['search_type'])) {
    $searchName = trim($_GET['search_name']);
    $searchType = $_GET['search_type'];

    if (!empty($searchName)) {
        $file = fopen($filename, 'r');
        if ($file) {
            $results = [];
            while (($line = fgets($file)) !== false) {
                list($firstName, $lastName) = explode(' ', trim($line), 2);
                if (($searchType === 'first_name' && stripos($firstName, $searchName) !== false) || 
                    ($searchType === 'last_name' && stripos($lastName, $searchName) !== false)) {
                    $results[] = $line;
                }
            }
            fclose($file);

            if (!empty($results)) {
                echo "<h3>Search Results:</h3>";
                echo "<ul>";
                foreach ($results as $result) {
                    echo "<li>" . htmlspecialchars($result) . "</li>";
                }
                echo "</ul>";
            } else {
                echo "No results found.";
            }
        } else {
            echo "Error reading the file.";
        }
    } else {
        echo "Please enter a name to search.";
    }
}

echo "php is functional!";
?>
