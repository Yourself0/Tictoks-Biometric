let quickSetup = localStorage.getItem('quickSetup') === 'true';
let url = '';

function quickSettings() {
    quickSetup = true;
    localStorage.setItem('quickSetup', 'true');
    url = "WifiPage";
    DeviceidShow('/WifiSetting');
}

function companyIdSettings() {
    console.log(quickSetup + " Quick setup");
    if (quickSetup || localStorage.getItem('quickSetup') === 'true') {
        url = "CompanyPage";
        DeviceidShow('/CompanySetting');
    } else {
        url = "";
        window.location.href = "/";
        localStorage.removeItem('quickSetup');
    }
}

function deviceIdSettings() {
    if (quickSetup || localStorage.getItem('quickSetup') === 'true') {
        url = "DeviceidPage";
        DeviceidShow('/Deviceid');
        localStorage.removeItem('quickSetup');
    } else {
        url = "";
        window.location.href = "/";
        localStorage.removeItem('quickSetup');
    }
}

function DeviceidShow(WebpageName) {
    Verification()
        .then(otp => {
            if (otp) {
                console.log('Received OTP:', otp);
                return ValidationCheck(otp,"Quick"); // Pass the OTP for validation
            } else {
                console.error('Received empty OTP');
                throw new Error('Empty OTP received');
            }
        })
        .then(status => {
            console.log('Validation status:', status);
            if (status >= 200 && status < 300) {
                window.location.href = WebpageName;
            } else {
                console.error('Validation failed with status:', status);
            }
        })
        .catch(error => {
            console.error('Error during validation:', error);
        });
}

function Verification() {
    return fetch('/OtpVerify')
        .then(response => {
            if (!response.ok) {
                console.error('Network response was not ok');
                throw new Error('Network response was not ok');
            }
            return response.text();
        })
        .catch(error => {
            console.error('Error fetching OTP:', error);
            throw error;
        });
}

function ValidationCheck(otp,url) {
    return new Promise((resolve, reject) => {
        var xhttp = new XMLHttpRequest();
        xhttp.open('POST', '/OtpVerificationChkQuick', true);
        xhttp.setRequestHeader('Content-Type', 'application/json');
        xhttp.onload = function () {
            if (xhttp.status >= 200 && xhttp.status < 300) {
                console.log('Connected');
                resolve(xhttp.status);
            } else {
                console.error('Request failed with status:', xhttp.status);
                reject(xhttp.status);
            }
        };
        xhttp.onerror = function () {
            console.error('Request failed:', xhttp.status);
            reject(xhttp.status);
        };
        xhttp.send(JSON.stringify({ OtpVerify: otp }));
    });
}


/*
let localStorage.getItem('quickSetup') === 'true';

function quickSettings() {
    quickSetup = true;
    localStorage.setItem('quickSetup', 'true');
    DeviceidShow('/WifiSetting');
}

function companyIdSettings() {
    console.log(quickSetup);
    if (quickSetup || localStorage.getItem('quickSetup') === 'true') {
        DeviceidShow('/CompanySetting');
    } else {
        window.location.href = '/';
    }
}

function deviceIdSettings() {
    if (quickSetup || localStorage.getItem('quickSetup') === 'true') {
        DeviceidShow('/Deviceid');
    } else {
        window.location.href = '/';
    }
}

function DeviceidShow(WebpageName) {
    Verification()
        .then(otp => {
            if (otp) {
                console.log('Received OTP:', otp);
                return ValidationCheck(otp); // Return the promise from ValidationCheck
            } else {
                console.error('Received empty OTP');
                // Handle empty OTP if needed
                throw new Error('Empty OTP received');
            }
        })
        .then(status => {
            console.log('Validation status:', status);
            if (status >= 200 && status < 300) {
                window.location.href = WebpageName;
                localStorage.removeItem('quickSetup');
                // Redirect or perform any action upon successful validation
            } else {
                console.error('Validation failed with status:', status);
                // Handle the failure appropriately
            }
        })
        .catch(error => {
            console.error('Error during validation:', error);
            // Handle verification or validation error if needed
        });
}

function Verification() {
    return fetch('/OtpVerify')
        .then(response => {
            if (!response.ok) {
                console.error('Network response was not ok');
                throw new Error('Network response was not ok');
            }
            return response.text();
        })
        .catch(error => {
            console.error('Error fetching OTP:', error);
            throw error; // Rethrow the error to propagate it to the caller
        });
}

function ValidationCheck(otp) {
    return new Promise((resolve, reject) => {
        var xhttp = new XMLHttpRequest();
        xhttp.open('POST', '/OtpVerificationChk', true);
        xhttp.setRequestHeader('Content-Type', 'application/json');
        xhttp.onload = function () {
            if (xhttp.status >= 200 && xhttp.status < 300) {
                console.log('Connected');
                resolve(xhttp.status);
            } else {
                console.error('Request failed with status:', xhttp.status);
                reject(xhttp.status); // Reject the promise with the status
            }
        };
        xhttp.onerror = function () {
            console.error('Request failed:', xhttp.status);
            reject(xhttp.status); // Reject the promise with the status
        };
        xhttp.send(JSON.stringify({ OtpVerify: otp }));
    });
}

*/