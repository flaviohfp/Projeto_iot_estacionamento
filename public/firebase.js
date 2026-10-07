import { initializeApp } from 'https://www.gstatic.com/firebasejs/12.19.0/firebase-app.js';

// Carregado apenas no modo Firebase; a demonstração funciona sem o SDK.
export const app = initializeApp(window.PARKING_CONFIG.firebase);
