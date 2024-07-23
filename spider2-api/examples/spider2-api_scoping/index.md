# Vitrix Software, s.r.o. - TestProj
## Specification
The goal is implement simple stopwatch application. You can use any front end library that suits you. To keep the application state use the Redux library. Make React components in reusable manner.

Commit as you go (in logical groups) and when you are done post us a GitHub repo to jan@vitrix.cz and time you spend on the problem.

### Basic features
* To be able to use up to 10 stopwatches simultaneously.
* To be able to select focused stopwatch that will become more visible at the screen
* Create component to Start - Pause - Reset stopwatch
* Stopwatch when started they should display in a format HH:MM:SS.sss
* Refresh their state on the screen in most effective way
* When stopwatches are not focused refresh their state only in second precision
* User should be able to give stop watches unique name
* Use flexbox or html grid to divide screen into useful regions

### Advanced features
* You can save stop watch state with the name to:
    * POST `https://example.vitrix.cz/api/<yourname> `
    * JSON format: `{"start_time": <number>, "elapsed_time": <number>, "name": <string>}`
    * The API allows to store 10 records that have unique names.
        * 200 - success
        * 417 - validation errors
* When the user refreshes the web page it should load up stored stop watches from API
    * GET `https://example.vitrix.cz/api/<yourname>`
    * It returns JSON:
        * `{ <record name> : {"start_time": <number>, "elapsed_time": <number>, "name": <string>}, ...}`

* User can remove record by API:
    * `DELETE https://example.vitrix.cz/api/<yourname>?name=<record name>`
    * 404 - when not found

  APIs returns status and error messages that should be presented to the end user.
    


