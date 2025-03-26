import http.client

# Open a connection to the server
conn = http.client.HTTPConnection("127.0.0.1", 8080)

# The data you want to send in chunks
chunk_files = ["chunk_aa", "chunk_ab", "chunk_ac", "chunk_ad", "chunk_ae", "chunk_af"]  # Add all the files you need here

# Prepare the headers for chunked transfer encoding
headers = {
    'Transfer-Encoding': 'chunked',
}

# Start the HTTP request with the specified headers
conn.request("POST", "/upload", body=None, headers=headers)

# Get the connection's socket
sock = conn.sock

# Loop over the chunk files
for chunk_file in chunk_files:
    # Read the content of the chunk file
    with open(chunk_file, 'rb') as file:
        chunk_data = file.read()
    
    # Send the chunk size in hexadecimal (uppercase 'X')
    sock.sendall(f"{len(chunk_data):X}\r\n".encode())  # Send chunk size in hex
    sock.sendall(chunk_data)  # Send the chunk data
    sock.sendall(b"\r\n")  # Send the chunk separator

# Send the final chunk (size 0 to indicate the end of the request body)
sock.sendall(b"0\r\n\r\n")  # End of chunks

# Close the connection
conn.close()

print("Chunked request sent successfully")