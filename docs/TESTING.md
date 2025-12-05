## Author
Bernardo Mota Coelho 
125059

Tiago Francisco Crespo do Vale
125913

### Test Case 1: Test GET requests for HTML, CSS & JS

*Steps:**
1. Add HTML, CSS and JavaScript files to `www` folder
2. Run: `make`
3. Run: `./server`
4. Access: `http://localhost:8080/name_of_your_html.html`
5. Verify that all components are working correctly.

**Expected Result:**
- The files are successfully added to the correct folder
- The URL `http://localhost:8080/name_of_your_html.html` should work correctly.


**Actual Result:** 
- A website appears

**Status:** x Pass ☐ Fail

**Screenshots:** 

![TestCase1](/docs/screenshots/testingitw.png)


### Test Case 2: Test GET requests for images

*Steps:**
1. Add a image to `www/images` folder
2. Run: `make`
3. Run: `./server`
4. Access: `http://localhost:8080/images/name_of_your_image`
5. Verify that image is showing correctly.

**Expected Result:**
- The image is successfully added to the correct folder
- The URL `http://localhost:8080/images/name_of_your_image` should work correctly.


**Actual Result:** 
- A image appears

**Status:** x Pass ☐ Fail

**Screenshots:** 

![TestCase2](/docs/screenshots/testingimage.png)

### Test Case 3: Verify correct HTTP status code 404

*Steps:**
1. Run: `make`
2. Run: `./server`
3. Access: `http://localhost:8080/images/unknown_file`
4. Verify that is showing `404 - File Not Found`

**Expected Result:**
- The program founds a error.
- The URL `http://localhost:8080/images/unknown_file`shows a 404.html with a message


**Actual Result:** 
- Error appears and programm doesn't close

**Status:** x Pass ☐ Fail

**Screenshots:** 

![TestCase3](/docs/screenshots/testing404.png)

### Test Case 4: Verify correct HTTP status code 403

*Steps:**
1. Create a directory without permissions
2. Run: `make`
3. Run: `./server`
4. Access: `http://localhost:8080/images/dir_without_perms`
5. Verify that is showing `403 - Acess Denied`

**Expected Result:**
- The program founds a error.
- The URL `http://localhost:8080/images/dir_without_perms` shows a 403.html with a message


**Actual Result:** 
- Error appears and program doesn't close

**Status:** x Pass ☐ Fail

**Screenshots:** 

![TestCase4](/docs/screenshots/testing403.png)

### Test Case 5: Test directory index serving

*Steps:**
1. Run: `make`
2. Run: `./server`
3. Access: `http://localhost:8080`
4. Verify that is showing `index.html`

**Expected Result:**
- The losthost connects do index.html.
- The URL `http://localhost:8080` shows the index.html


**Actual Result:** 
- Showing index.html when it's not provided a file

**Status:** x Pass ☐ Fail

**Screenshots:** 

![TestCase5](/docs/screenshots/testingindex.png)


### Test Case 6: Verify correct Content-Type headers

*Steps:**
1. Run: `make`
2. Run: `./server`
3. Type on terminal: `curl -I http://localhost:8080/your_file_name`
4. Verify that is showing the type of the file

**Expected Result:**
- The program recognize the file types
- The URL `http://localhost:8080/images/your_file_name` handles various type of files


**Actual Result:** 
- The program handle various types of files

**Status:** x Pass ☐ Fail

**Screenshots:** 

![TestCase6](/docs/screenshots/testingheaders12.png)