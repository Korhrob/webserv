<?php

$filename = __DIR__ . '/people.txt';
$status = 1;

if ($_SERVER['DUP'] === 'FAIL')
{
	exit;
}
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_SERVER['first_name'], $_SERVER['last_name']))
{
	$status = 0;
}
else if ($_SERVER['REQUEST_METHOD'] === 'GET' && isset($_SERVER['search_name'], $_SERVER['search_type']))
{
	$status = 0;
}
if ($status === 1)
	exit;

while(1) {};

echo '<!DOCTYPE html>';
echo '<html lang="en">';
echo '<head>';
echo '    <meta charset="UTF-8">';
echo '    <meta name="viewport" content="width=device-width, initial-scale=1.0">';
echo '    <title>People Form</title>';
echo '    <style>';
echo '        body {';
echo '            font-family: Arial, sans-serif;';
echo '            background-color: #232020;';
echo '            display: flex;';
echo '            justify-content: center;';
echo '            align-items: center;';
echo '            height: 100vh;';
echo '            margin: 0;';
echo '            color: white;';
echo '        }';
echo '';
echo '        #content {';
echo '            display: flex;';
echo '            flex-direction: column;';
echo '            align-items: center;';
echo '            width: 350px;';
echo '        }';
echo '';
echo '        h2 {';
echo '            text-align: center;';
echo '            margin-bottom: 15px;';
echo '        }';
echo '';
echo '        form {';
echo '            display: flex;';
echo '            flex-direction: column;';
echo '            align-items: center;';
echo '            width: 100%;';
echo '        }';
echo '';
echo '        label {';
echo '            align-self: flex-start;';
echo '            margin-bottom: 5px;';
echo '        }';
echo '';
echo '        input {';
echo '            width: 100%;';
echo '            padding: 8px;';
echo '            margin-bottom: 10px;';
echo '            font-size: 16px;';
echo '        }';
echo '';
echo '        .button {';
echo '            width: 100%;';
echo '            padding: 10px;';
echo '            font-size: 16px;';
echo '            background-color: #4CAF50;';
echo '            color: white;';
echo '            border: none;';
echo '            border-radius: 5px;';
echo '            cursor: pointer;';
echo '            margin-top: 10px;';
echo '        }';
echo '';
echo '        .button:hover {';
echo '            background-color: #45a049;';
echo '        }';
echo '';
echo '        #peopleButton {';
echo '            background-color: white;';
echo '        }';
echo '';
echo '        .message {';
echo '            font-size: 16px;';
echo '            color: white;';
echo '            margin-top: 10px;';
echo '            min-height: 24px;';
echo '            opacity: 0;';
echo '            transition: opacity 0.3s ease-in-out;';
echo '        }';
echo '';
echo '        hr {';
echo '            width: 100%;';
echo '            border: 0;';
echo '            height: 1px;';
echo '            background: white;';
echo '            margin: 20px 0;';
echo '        }';
echo '';
echo '        .buttonindex {';
echo '            position: absolute;';
echo '            bottom: 20px;';
echo '            left: 50%;';
echo '            transform: translateX(-50%);';
echo '            padding: 10px 20px;';
echo '            background-color: #4CAF50;';
echo '            color: white;';
echo '            border: none;';
echo '            border-radius: 5px;';
echo '            font-size: 16px;';
echo '        }';
echo '';
echo '        .buttonindex:hover {';
echo '            background-color: #45a049;';
echo '        }';
echo '    </style>';
echo '</head>';
echo '<body>';
echo '    <div id="content">';
echo '';
echo '        <!-- Add Person Section -->';
echo '        <h2>Add Person</h2>';
echo '        <form action="" method="post" enctype="multipart/form-data">';
echo '            <label for="first_name">First Name:</label>';
echo '            <input type="text" id="first_name" name="first_name" required>';
echo '';
echo '            <label for="last_name">Last Name:</label>';
echo '            <input type="text" id="last_name" name="last_name" required>';
echo '';
echo '            <button type="submit" class="peopleButton">Add Person</button>';
echo '            <p id="addPersonMessage" class="message"></p>';
echo '        </form>';
echo '';

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

echo '        <hr>';
echo '';
echo '        <!-- Search for Person Section -->';
echo '        <h2>Search for Person</h2>';
echo '        <form action="" method="get" enctype="multipart/form-data">';
echo '            <label for="search_name">First or Last Name:</label>';
echo '            <input type="text" id="search_name" name="search_name" required>';
echo '';
echo '            <div style="display: flex; flex-direction: column; margin: 10px 0;">';
echo '                <input type="radio" id="first_name_search" name="search_type" value="first_name" checked>';
echo '                <label for="first_name_search">First Name</label>';
echo '';
echo '                <input type="radio" id="last_name_search" name="search_type" value="last_name">';
echo '                <label for="last_name_search">Last Name</label>';
echo '            </div>';
echo '';
echo '            <button type="submit" class="peopleButton">Search</button>';
echo '            <p id="searchMessage" class="message"></p>';
echo '        </form>';

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
            if (!empty($results)) {
				echo "<h3>Search Results:</h3>";
				echo "<ul style='list-style-type: none; padding: 0; text-align: center; margin: 0 auto; width: fit-content;'>";
				foreach ($results as $result) {
					echo "<li>" . htmlspecialchars($result) . "</li>";
				}
				echo "</ul>";
			} else {
				echo "No results found.";
			}
        } else {echo "Error reading the file.";}
    } else {echo "Please enter a name to search.";}
}

echo '';
echo '        <button class="buttonindex" onclick="window.location.href=\'/index\'">Back to Index</button>';
echo '    </div>';
echo '</body>';
echo '</html>';