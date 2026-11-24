/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        ink: { 950: "#07090d", 900: "#0c1017", 800: "#121822", 700: "#1b2330", 600: "#2a3546" },
        accent: { DEFAULT: "#22d3ee", dim: "#0e7490" },
      },
      fontFamily: { mono: ["JetBrains Mono", "ui-monospace", "monospace"] },
    },
  },
  plugins: [],
};
