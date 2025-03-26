<?php

$filename = __DIR__ . '/people.txt';
$status = 0;

sleep(10);

echo "<!DOCTYPE html>";
echo "<html lang=\"en\">";
echo "<head>";
echo "<title>Webserv</title>";
echo "<meta charset=\"UTF-8\">";
echo "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
echo "<title>Nested Multipart File Upload Form</title>";
echo "<style>";
echo "    .button {";
echo "    position: absolute;";
echo "    bottom: 20px;";
echo "    left: 50%;";
echo "    transform: translateX(-50%);";
echo "    padding: 10px 20px;";
echo "    background-color: #4CAF50;";
echo "    color: white;";
echo "    border: none;";
echo "    border-radius: 5px;";
echo "    font-size: 16px;";
echo "    }";
echo "</style>";
echo "</head>";
echo "<body>";
echo "    <h1>People Database</h1>";
echo "    <!-- Form to add new people -->";
echo "    <h2>Add Person</h2>";
echo "    <form action=\"people.cgi.php\" method=\"post\" enctype=\"multipart/form-data\">";
echo "        <label for=\"first_name\">First Name:</label>";
echo "        <input type=\"text\" id=\"first_name\" name=\"first_name\" required><br>";
echo "        <label for=\"last_name\">Last Name:</label>";
echo "        <input type=\"text\" id=\"last_name\" name=\"last_name\" required><br>";
echo "        <button type=\"submit\">Add Person</button>";
echo "    </form>";
echo "    <hr>";

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_SERVER['first_name'], $_SERVER['last_name'])) {
    $firstName = trim($_SERVER['first_name']);
    $lastName = trim($_SERVER['last_name']);

	if (strpos($firstName, ' ') !== false)
	{
		echo "[$firstName] cannot contain spaces!";
		$status = 1;
	}
	if ($status === 0 && strpos($lastName, ' ') !== false)
	{
		echo "[$lastName] cannot contain spaces!";
		$status = 1;
	}

    if ($status === 0 && !empty($firstName) && !empty($lastName)) 
	{
        $file = fopen($filename, 'r');
		while (($line = fgets($file)) !== false) 
		{
			$line = trim($line);
			list($fileFirstName, $fileLastName) = explode(" ", $line);
			if ($fileFirstName === $firstName && $fileLastName === $lastName) {
				echo "Person $firstName $lastName already exists in the database.";
				fclose($file);
				$status = 1;
				break ;
			}
		}
		if ($status === 0)
		{
			$file = fopen($filename, 'a');
			if ($file) 
			{
				fwrite($file, $firstName . ' ' . $lastName . "\n");
				fclose($file);
				echo "Person added successfully!";
			} else {
				echo "Error saving data!";
			}
		}
    } else {
        echo "First and Last names are required.";
    }
}

echo "    <!-- Section for displaying search results -->";
echo "    <div id=\"Post results\">";
echo "    </div>";
echo "    <hr>";
echo "    <!-- Form to search people by first or last name -->";
echo "    <h2>Search for Person</h2>";
echo "    <form action=\"people.cgi.php\" method=\"get\" enctype=\"multipart/form-data\">";
echo "        <label for=\"search_name\">Search by First or Last Name:</label>";
echo "        <input type=\"text\" id=\"search_name\" name=\"search_name\" required><br>";
echo "        <input type=\"radio\" id=\"first_name_search\" name=\"search_type\" value=\"first_name\" checked>";
echo "        <label for=\"first_name_search\">First Name</label>";
echo "        <input type=\"radio\" id=\"last_name_search\" name=\"search_type\" value=\"last_name\">";
echo "        <label for=\"last_name_search\">Last Name</label><br>";
echo "        <button type=\"submit\">Search</button>";
echo "    </form>";
echo "    <hr>";

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

echo "    <!-- Section for displaying search results -->";
echo "    <div id=\"search_results\">";
echo "    </div>";
echo "    <button class=\"button\" onclick=\"window.location.href='/index'\">Back to Index</button>";
echo "</body>";
echo "</html>";

?>
