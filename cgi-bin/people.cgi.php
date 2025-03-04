<?php

$filename = __DIR__ . '/people.txt';

// echo "<br><br>";
// print_r($_SERVER);
// echo "<br><br>";
// print_r(getenv());
// echo "<br><br>";
// print_r($_POST);
// echo "<br><br>";

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_SERVER['first_name'], $_SERVER['last_name'])) {
    $firstName = trim($_SERVER['first_name']);
    $lastName = trim($_SERVER['last_name']);

	if (strpos($firstName, ' ') !== false)
	{
		echo "[$firstName] cannot contain spaces!";
		return ;
	}
	if (strpos($lastName, ' ') !== false)
	{
		echo "[$lastName] cannot contain spaces!";
		return ;
	}

    if (!empty($firstName) && !empty($lastName)) 
	{
        $file = fopen($filename, 'r');
		while (($line = fgets($file)) !== false) 
		{
			$line = trim($line);
			list($fileFirstName, $fileLastName) = explode(" ", $line);
			if ($fileFirstName === $firstName && $fileLastName === $lastName) {
				echo "Person $firstName $lastName already exists in the database.";
				fclose($file);
				return ;
			}
		}
		$file = fopen($filename, 'a');
        if ($file) 
		{
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

if ($_SERVER['REQUEST_METHOD'] === 'GET' && isset($_SERVER['search_name'], $_SERVER['search_type'])) {
    $searchName = trim($_SERVER['search_name']);
    $searchType = $_SERVER['search_type'];

    if (!empty($searchName)) {
        $file = fopen($filename, 'r');
        if ($file) {
            $results = [];
            while (($line = fgets($file)) !== false) {
                list($firstName, $lastName) = explode(' ', trim($line), 2);
				$firstNameMatch = preg_match('/\b' . preg_quote($searchName, '/') . '\b/i', $firstName);
                $lastNameMatch = preg_match('/\b' . preg_quote($searchName, '/') . '\b/i', $lastName);
                if (($searchType === 'first_name' && $firstNameMatch) || 
                    ($searchType === 'last_name' && $lastNameMatch)) {
                    $results[] = $line;
                }
            }
            fclose($file);
            if (!empty($results))
			{
                echo "<h3>Search Results:</h3>";
                echo "<ul>";
                foreach ($results as $result) {
                    echo "<li>" . htmlspecialchars($result) . "</li>";
                }
                echo "</ul>";
            } else {echo "No results found.";}
        } else {echo "Error reading the file.";}
    } else {echo "Please enter a name to search.";}
}
?>
