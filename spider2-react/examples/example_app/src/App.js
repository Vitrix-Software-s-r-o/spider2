import React from 'react';
import logo from './logo.svg';
import './App.css';

function useServerTime() {
    const [time, setTime] = React.useState("");
    React.useEffect(() => {
        const interval = setInterval(() => {

            // Fetch the time from the server
            fetch('./api/time').then((res) => res.arrayBuffer()).then((data) => {
                const encoder = new TextDecoder();
                setTime(encoder.decode(data, {stream: false}));
            });

        }, 100);
        return () => clearInterval(interval);
    }, []);
    return time;


}

function App() {
    const serverTime = useServerTime();

    return (
        <div className="App">
            <header className="App-header">
                <img src={logo} className="App-logo" alt="logo"/>
                <p>
                    Edit <code>src/App.js</code> and save to reload. Just test :)
                </p>
                <p>
                    {serverTime}
                </p>
                <a
                    className="App-link"
                    href="https://reactjs.org"
                    target="_blank"
                    rel="noopener noreferrer"
                >
                    Learn React
                </a>
            </header>
        </div>
    );
}

export default App;
