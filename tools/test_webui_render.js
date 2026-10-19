#!/usr/bin/env node
"use strict";
// Material 3 WebUI integration contract. The current UI uses a different DOM contract than the
// legacy control dashboard; this test checks the actual shipped IDs, selectors, fixed API bridge,
// and safe DOM construction. The status test separately parses the C++-generated fixture.
const fs = require("fs");
const path = require("path");
const root = path.resolve(__dirname, "..");
const dir = process.argv[2];
if (!dir) { console.error("usage: test_webui_render.js <fixture-dir>"); process.exit(2); }
const web = path.join(root, "module/webroot");
const html = fs.readFileSync(path.join(web, "index.html"), "utf8");
const app = fs.readFileSync(path.join(web, "app.js"), "utf8");
const control = fs.readFileSync(path.join(web, "control_logic.js"), "utf8");
const status = fs.readFileSync(path.join(web, "engine_status.js"), "utf8");
const fixture = JSON.parse(fs.readFileSync(path.join(dir, "ready.json"), "utf8"));
let checks = 0, failures = 0;
function check(ok, label) { checks++; if (!ok) { failures++; console.error("FAIL: " + label); } }
const ids = new Set([...html.matchAll(/\bid="([^"]+)"/g)].map(m => m[1]));
const usedIds = new Set([...app.matchAll(/\$\("#([A-Za-z0-9_-]+)"\)/g)].map(m => m[1]));
for (const id of usedIds) check(ids.has(id), "app selector has matching DOM id: " + id);
check(fixture && typeof fixture === "object" && Number.isFinite(fixture.updated_epoch), "C++ status fixture is available");
check(/function render\(p\)/.test(app) && /function renderResources\(p\)/.test(app) && /function renderDecision\(p\)/.test(app), "runtime and dashboard renderers exist");
check(!/innerHTML|insertAdjacentHTML|outerHTML|document\.write|\beval\(/.test(app + control), "dynamic UI never injects HTML");
check(/document\.createElement/.test(app) && /textContent/.test(app), "dynamic data uses DOM nodes and textContent");
check(/data-page="overview"/.test(html) && /data-page="resources"/.test(html) && /data-page="decisions"/.test(html) && /data-page="policy"/.test(html) && /data-page="journal"/.test(html) && /data-page="system"/.test(html), "all six Material 3 pages are reachable");
for (const id of ["btn-start-engine","btn-stop-engine","btn-restart-engine","btn-rediscover","btn-apply-mode","btn-refresh"]) check(ids.has(id) && app.includes(id), "control is present and wired: " + id);
check(/C\.snapshotCmd\(\)/.test(app) && /C\.writeConfigCmd\(/.test(app) && /C\.gate\(/.test(app), "runtime status and policy use fixed CoreFlow command builders");
check(/SAFE_MODE/.test(app) && /mutation_mode/.test(control) && /mutation_armed/.test(control), "mode changes retain explicit policy and safety gates");
check(/coreflow:material-ready/.test(html) && /coreflow:material-fallback/.test(html) && /material-fallback\.css/.test(html), "CDN enhancement and local fallback are wired");
check(/mutation_mode=observe|mutation_mode.*observe/.test(control) && /selected" data-mode="observe/.test(html), "Observe is the UI default");
console.log("CoreFlow WebUI Material 3 render contract: " + (checks - failures) + "/" + checks + " checks passed");
process.exit(failures ? 1 : 0);
