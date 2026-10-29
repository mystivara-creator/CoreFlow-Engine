#!/usr/bin/env node
"use strict";
// Consolidated React/TypeScript KernelSU WebUI integration contract.
// Verifies that the offline module/webroot bundle, fixed C++ command builders,
// safety gates, and zero-CDN asset rules match coreflowd v3.0.0.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(__dirname, "..");
const dir = process.argv[2];
if (!dir) {
  console.error("usage: test_webui_render.js <fixture-dir>");
  process.exit(2);
}

const web = path.join(root, "module/webroot");
const html = fs.readFileSync(path.join(web, "index.html"), "utf8");
const app = fs.readFileSync(path.join(web, "app.js"), "utf8");
const css = fs.readFileSync(path.join(web, "style.css"), "utf8");
const control = fs.readFileSync(path.join(web, "control_logic.js"), "utf8");
const status = fs.readFileSync(path.join(web, "engine_status.js"), "utf8");
const fixture = JSON.parse(fs.readFileSync(path.join(dir, "ready.json"), "utf8"));

function walkTs(d) {
  if (!fs.existsSync(d)) return [];
  return fs.readdirSync(d, { withFileTypes: true }).flatMap((entry) => {
    const full = path.join(d, entry.name);
    if (entry.isDirectory()) return walkTs(full);
    return /\.(ts|tsx)$/.test(entry.name) ? [fs.readFileSync(full, "utf8")] : [];
  });
}

const webuiSource = walkTs(path.join(root, "webui")).join("\n");

let checks = 0;
let failures = 0;
function check(ok, label) {
  checks++;
  if (!ok) {
    failures++;
    console.error("FAIL: " + label);
  }
}

check(fixture && typeof fixture === "object" && Number.isFinite(fixture.updated_epoch), "C++ status fixture is available");
check(/id="root"/.test(html), "index.html exposes #root mount container");
check(/\.\/engine_status\.js/.test(html) && /\.\/control_logic\.js/.test(html), "index.html loads engine_status.js and control_logic.js locally");
check(/\.\/app\.js/.test(html) && /\.\/style\.css/.test(html), "index.html loads offline ./app.js and ./style.css");
check(!/https?:\/\/(cdn\.|fonts\.googleapis\.com|fonts\.gstatic\.com|unpkg\.com)/i.test(html), "index.html contains zero external runtime CDN URLs");
check(app.length > 1000 && css.length > 500 && status.length > 500, "compiled app.js, style.css, and engine_status.js are non-empty");
check(
  !/dangerouslySetInnerHTML|\.innerHTML\s*=|insertAdjacentHTML|outerHTML|document\.write|\beval\(/.test(
    webuiSource + "\n" + control
  ),
  "WebUI TypeScript source and control_logic.js never inject raw HTML or call eval"
);

for (const id of ["btn-start-engine", "btn-stop-engine", "btn-restart-engine", "btn-rediscover"]) {
  check(app.includes(id), "daemon lifecycle control is compiled into app.js: " + id);
}

check(
  /snapshotCmd\(\)/.test(webuiSource) &&
    /writeConfigCmd\(/.test(webuiSource) &&
    /\.gate\(/.test(webuiSource) &&
    /startCmd\(\)/.test(webuiSource) &&
    /stopCmd\(\)/.test(webuiSource) &&
    /restartCmd\(\)/.test(webuiSource) &&
    /refreshCmd\(\)/.test(webuiSource),
  "WebUI uses fixed CoreFlowControl shell command builders and safety gate"
);

check(
  /SAFE_MODE/.test(webuiSource) && /mutation_mode/.test(control) && /mutation_armed/.test(control),
  "automatic operation retains compatibility authority and SAFE_MODE handling"
);
check(/mutation_mode: "adaptive"/.test(control) && !/Standby \(Mutation Disabled\)/.test(webuiSource), "automatic operation is default and the Standby selector is removed");

console.log("CoreFlow WebUI render contract: " + (checks - failures) + "/" + checks + " checks passed");
process.exit(failures ? 1 : 0);
