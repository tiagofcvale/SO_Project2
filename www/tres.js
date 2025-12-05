// Simple message in the console to confirm the file is loading
console.log("Heart animation script loaded! ❤️");

// Optional: change animation speed dynamically every few seconds
setInterval(() => {
    const heart = document.getElementById("heart");
    const newDuration = (Math.random() * 0.5 + 0.8).toFixed(2); // between 0.8s–1.3s
    heart.style.animationDuration = newDuration + "s";
}, 2000);
