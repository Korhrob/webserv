<?php

if ($_SERVER['REQUEST_METHOD'] == 'POST' && isset($_FILES['file']))
{
	$file = $_FILES['file'];

	if ($file['error'] !== UPLOAD_ERR_OK)
	{
		echo "Error during file upload: " . $file['error'];
		exit;
	}

	$maxSize = 5 * 1024 * 1024;
	if ($file['size'] > $maxSize)
	{
		echo "File is too large. Maximum size is 5MB.";
		exit;
	}

	$uploadDir = '../uploads/';
	$uploadFile = $uploadDir . basename($file['name']);

	if (file_exists($uploadFile))
	{
		echo "File already exists!";
		exit;
	}

	if (move_uploaded_file($file['tmp_name'], $uploadFile))
	{
		echo "File succesfully uploaded!";
	}
	else
	{
		echo "Failed to upload file.";
	}
}
else
{
	echo "No file uploaded or invalid request method.";
}

?>