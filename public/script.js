// Initialize Firebase
try {
    firebase.initializeApp(firebaseConfig);
    const database = firebase.database();
    console.log("Firebase initialized successfully");
    
    // Reference to the luminance data in the database
    const luxRef = database.ref('luminance');
    
    // Listen for real-time updates
    luxRef.on('value', (snapshot) => {
        console.log("Luminance data received:", snapshot.val());
        const data = snapshot.val();
        if (data && data.lux !== undefined) {
            document.getElementById('luxValue').textContent = data.lux.toFixed(2);
            document.getElementById('timestamp').textContent = new Date(data.timestamp || Date.now()).toLocaleString();
        } else {
            document.getElementById('luxValue').textContent = 'No data';
            document.getElementById('timestamp').textContent = 'N/A';
            console.log("No luminance data found in database");
        }
    }, (error) => {
        console.error('Error reading from database:', error);
        document.getElementById('luxValue').textContent = 'Error';
    });
    
    // Reference to the logs in the database
    const logsRef = database.ref('logs');
    
    // Optional: clear the placeholder once the first log arrives
    let logsInitialized = false;
    
    // Listen for real-time log updates
    logsRef.on('child_added', (snapshot) => {
        console.log("New log received:", snapshot.val());
        const log = snapshot.val();
        const logsContainer = document.getElementById('logsContainer');
        if (!logsInitialized) {
            logsContainer.innerHTML = '';
            logsInitialized = true;
        }
        const logElement = document.createElement('p');
        logElement.textContent = new Date(log.timestamp).toLocaleString() + ': ' + log.message;
        logsContainer.appendChild(logElement);
        logsContainer.scrollTop = logsContainer.scrollHeight;  // Auto-scroll to bottom
    }, (error) => {
        console.error('Error reading logs:', error);
    });

} catch (error) {
    console.error('Error initializing Firebase:', error);
    document.getElementById('luxValue').textContent = 'Init Error';
}
