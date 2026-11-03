import fs from 'node:fs';
import path from 'node:path';
import tailwindcss from '@tailwindcss/vite';
import react from '@vitejs/plugin-react';
import { defineConfig, Plugin } from 'vite';

// Mirrors the production web bundle into module/webroot so both standard web
// deployments (dist/) and KernelSU/Magisk Android packaging stay synchronized.
function syncModuleWebroot(): Plugin {
  return {
    name: 'sync-module-webroot',
    closeBundle() {
      const distDir = path.resolve('dist');
      const webrootDir = path.resolve('module/webroot');
      if (!fs.existsSync(distDir)) return;
      fs.rmSync(webrootDir, { recursive: true, force: true });
      fs.mkdirSync(webrootDir, { recursive: true });
      fs.cpSync(distDir, webrootDir, { recursive: true });
    },
  };
}

export default defineConfig({
  base: './',
  publicDir: 'webui/public',
  plugins: [react(), tailwindcss(), syncModuleWebroot()],
  server: {
    host: '0.0.0.0',
    port: 3000,
    allowedHosts: true,
    hmr: process.env.DISABLE_HMR !== 'true',
  },
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    assetsDir: '.',
    cssCodeSplit: false,
    rollupOptions: {
      output: {
        entryFileNames: 'app.js',
        chunkFileNames: '[name].js',
        assetFileNames: (assetInfo) => {
          if (assetInfo.name && assetInfo.name.endsWith('.css')) {
            return 'style.css';
          }
          return '[name][extname]';
        },
      },
    },
  },
});
