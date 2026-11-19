var mh=Object.defineProperty;var fh=(o,c,m)=>c in o?mh(o,c,{enumerable:!0,configurable:!0,writable:!0,value:m}):o[c]=m;var Hc=(o,c,m)=>fh(o,typeof c!="symbol"?c+"":c,m);(function(){const c=document.createElement("link").relList;if(c&&c.supports&&c.supports("modulepreload"))return;for(const j of document.querySelectorAll('link[rel="modulepreload"]'))d(j);new MutationObserver(j=>{for(const O of j)if(O.type==="childList")for(const N of O.addedNodes)N.tagName==="LINK"&&N.rel==="modulepreload"&&d(N)}).observe(document,{childList:!0,subtree:!0});function m(j){const O={};return j.integrity&&(O.integrity=j.integrity),j.referrerPolicy&&(O.referrerPolicy=j.referrerPolicy),j.crossOrigin==="use-credentials"?O.credentials="include":j.crossOrigin==="anonymous"?O.credentials="omit":O.credentials="same-origin",O}function d(j){if(j.ep)return;j.ep=!0;const O=m(j);fetch(j.href,O)}})();var Gc={exports:{}},gl={};/**
 * @license React
 * react-jsx-runtime.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Uf;function ph(){if(Uf)return gl;Uf=1;var o=Symbol.for("react.transitional.element"),c=Symbol.for("react.fragment");function m(d,j,O){var N=null;if(O!==void 0&&(N=""+O),j.key!==void 0&&(N=""+j.key),"key"in j){O={};for(var L in j)L!=="key"&&(O[L]=j[L])}else O=j;return j=O.ref,{$$typeof:o,type:d,key:N,ref:j!==void 0?j:null,props:O}}return gl.Fragment=c,gl.jsx=m,gl.jsxs=m,gl}var Bf;function vh(){return Bf||(Bf=1,Gc.exports=ph()),Gc.exports}var r=vh(),Ic={exports:{}},le={};/**
 * @license React
 * react.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Vf;function yh(){if(Vf)return le;Vf=1;var o=Symbol.for("react.transitional.element"),c=Symbol.for("react.portal"),m=Symbol.for("react.fragment"),d=Symbol.for("react.strict_mode"),j=Symbol.for("react.profiler"),O=Symbol.for("react.consumer"),N=Symbol.for("react.context"),L=Symbol.for("react.forward_ref"),B=Symbol.for("react.suspense"),R=Symbol.for("react.memo"),x=Symbol.for("react.lazy"),g=Symbol.for("react.activity"),q=Symbol.for("react.view_transition"),$=Symbol.iterator;function M(p){return p===null||typeof p!="object"?null:(p=$&&p[$]||p["@@iterator"],typeof p=="function"?p:null)}var X={isMounted:function(){return!1},enqueueForceUpdate:function(){},enqueueReplaceState:function(){},enqueueSetState:function(){}},I=Object.assign,J={};function ae(p,D,W){this.props=p,this.context=D,this.refs=J,this.updater=W||X}ae.prototype.isReactComponent={},ae.prototype.setState=function(p,D){if(typeof p!="object"&&typeof p!="function"&&p!=null)throw Error("takes an object of state variables to update or a function which returns an object of state variables.");this.updater.enqueueSetState(this,p,D,"setState")},ae.prototype.forceUpdate=function(p){this.updater.enqueueForceUpdate(this,p,"forceUpdate")};function F(){}F.prototype=ae.prototype;function re(p,D,W){this.props=p,this.context=D,this.refs=J,this.updater=W||X}var xe=re.prototype=new F;xe.constructor=re,I(xe,ae.prototype),xe.isPureReactComponent=!0;var C=Array.isArray;function z(){}var P={H:null,A:null,T:null,S:null},ye=Object.prototype.hasOwnProperty;function Re(p,D,W){var ee=W.ref;return{$$typeof:o,type:p,key:D,ref:ee!==void 0?ee:null,props:W}}function Me(p,D){return Re(p.type,D,p.props)}function Ne(p){return typeof p=="object"&&p!==null&&p.$$typeof===o}function $e(p){var D={"=":"=0",":":"=2"};return"$"+p.replace(/[=:]/g,function(W){return D[W]})}var it=/\/+/g;function Le(p,D){return typeof p=="object"&&p!==null&&p.key!=null?$e(""+p.key):D.toString(36)}function H(p){switch(p.status){case"fulfilled":return p.value;case"rejected":throw p.reason;default:switch(typeof p.status=="string"?p.then(z,z):(p.status="pending",p.then(function(D){p.status==="pending"&&(p.status="fulfilled",p.value=D)},function(D){p.status==="pending"&&(p.status="rejected",p.reason=D)})),p.status){case"fulfilled":return p.value;case"rejected":throw p.reason}}throw p}function ne(p,D,W,ee,pe){var ve=typeof p;(ve==="undefined"||ve==="boolean")&&(p=null);var ge=!1;if(p===null)ge=!0;else switch(ve){case"bigint":case"string":case"number":ge=!0;break;case"object":switch(p.$$typeof){case o:case c:ge=!0;break;case x:return ge=p._init,ne(ge(p._payload),D,W,ee,pe)}}if(ge)return pe=pe(p),ge=ee===""?"."+Le(p,0):ee,C(pe)?(W="",ge!=null&&(W=ge.replace(it,"$&/")+"/"),ne(pe,D,W,"",function(E){return E})):pe!=null&&(Ne(pe)&&(pe=Me(pe,W+(pe.key==null||p&&p.key===pe.key?"":(""+pe.key).replace(it,"$&/")+"/")+ge)),D.push(pe)),1;ge=0;var A=ee===""?".":ee+":";if(C(p))for(var U=0;U<p.length;U++)ee=p[U],ve=A+Le(ee,U),ge+=ne(ee,D,W,ve,pe);else if(U=M(p),typeof U=="function")for(p=U.call(p),U=0;!(ee=p.next()).done;)ee=ee.value,ve=A+Le(ee,U++),ge+=ne(ee,D,W,ve,pe);else if(ve==="object"){if(typeof p.then=="function")return ne(H(p),D,W,ee,pe);throw D=String(p),Error("Objects are not valid as a React child (found: "+(D==="[object Object]"?"object with keys {"+Object.keys(p).join(", ")+"}":D)+"). If you meant to render a collection of children, use an array instead.")}return ge}function Z(p,D,W){if(p==null)return p;var ee=[],pe=0;return ne(p,ee,"","",function(ve){return D.call(W,ve,pe++)}),ee}function Ee(p){if(p._status===-1){var D=p._result,W=D();W.then(function(ee){(p._status===0||p._status===-1)&&(p._status=1,p._result=ee,W.status===void 0&&(W.status="fulfilled",W.value=ee))},function(ee){(p._status===0||p._status===-1)&&(p._status=2,p._result=ee,W.status===void 0&&(W.status="rejected",W.reason=ee))}),p._status===-1&&(p._status=0,p._result=W)}if(p._status===1)return p._result.default;throw p._result}var he=typeof reportError=="function"?reportError:function(p){if(typeof window=="object"&&typeof window.ErrorEvent=="function"){var D=new window.ErrorEvent("error",{bubbles:!0,cancelable:!0,message:typeof p=="object"&&p!==null&&typeof p.message=="string"?String(p.message):String(p),error:p});if(!window.dispatchEvent(D))return}else if(typeof process=="object"&&typeof process.emit=="function"){process.emit("uncaughtException",p);return}console.error(p)};function gt(p){var D=P.T,W={};W.types=D!==null?D.types:null,P.T=W;try{var ee=p(),pe=P.S;pe!==null&&pe(W,ee),typeof ee=="object"&&ee!==null&&typeof ee.then=="function"&&ee.then(z,he)}catch(ve){he(ve)}finally{D!==null&&W.types!==null&&(D.types=W.types),P.T=D}}function bt(p){var D=P.T;if(D!==null){var W=D.types;W===null?D.types=[p]:W.indexOf(p)===-1&&W.push(p)}else gt(bt.bind(null,p))}var xt={map:Z,forEach:function(p,D,W){Z(p,function(){D.apply(this,arguments)},W)},count:function(p){var D=0;return Z(p,function(){D++}),D},toArray:function(p){return Z(p,function(D){return D})||[]},only:function(p){if(!Ne(p))throw Error("React.Children.only expected to receive a single React element child.");return p}};return le.Activity=g,le.Children=xt,le.Component=ae,le.Fragment=m,le.Profiler=j,le.PureComponent=re,le.StrictMode=d,le.Suspense=B,le.ViewTransition=q,le.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE=P,le.__COMPILER_RUNTIME={__proto__:null,c:function(p){return P.H.useMemoCache(p)}},le.addTransitionType=bt,le.cache=function(p){return function(){return p.apply(null,arguments)}},le.cacheSignal=function(){return null},le.cloneElement=function(p,D,W){if(p==null)throw Error("The argument must be a React element, but you passed "+p+".");var ee=I({},p.props),pe=p.key;if(D!=null)for(ve in D.key!==void 0&&(pe=""+D.key),D)!ye.call(D,ve)||ve==="key"||ve==="__self"||ve==="__source"||ve==="ref"&&D.ref===void 0||(ee[ve]=D[ve]);var ve=arguments.length-2;if(ve===1)ee.children=W;else if(1<ve){for(var ge=Array(ve),A=0;A<ve;A++)ge[A]=arguments[A+2];ee.children=ge}return Re(p.type,pe,ee)},le.createContext=function(p){return p={$$typeof:N,_currentValue:p,_currentValue2:p,_threadCount:0,Provider:null,Consumer:null},p.Provider=p,p.Consumer={$$typeof:O,_context:p},p},le.createElement=function(p,D,W){var ee,pe={},ve=null;if(D!=null)for(ee in D.key!==void 0&&(ve=""+D.key),D)ye.call(D,ee)&&ee!=="key"&&ee!=="__self"&&ee!=="__source"&&(pe[ee]=D[ee]);var ge=arguments.length-2;if(ge===1)pe.children=W;else if(1<ge){for(var A=Array(ge),U=0;U<ge;U++)A[U]=arguments[U+2];pe.children=A}if(p&&p.defaultProps)for(ee in ge=p.defaultProps,ge)pe[ee]===void 0&&(pe[ee]=ge[ee]);return Re(p,ve,pe)},le.createRef=function(){return{current:null}},le.forwardRef=function(p){return{$$typeof:L,render:p}},le.isValidElement=Ne,le.lazy=function(p){return{$$typeof:x,_payload:{_status:-1,_result:p},_init:Ee}},le.memo=function(p,D){return{$$typeof:R,type:p,compare:D===void 0?null:D}},le.startTransition=gt,le.unstable_useCacheRefresh=function(){return P.H.useCacheRefresh()},le.use=function(p){return P.H.use(p)},le.useActionState=function(p,D,W){return P.H.useActionState(p,D,W)},le.useCallback=function(p,D){return P.H.useCallback(p,D)},le.useContext=function(p){return P.H.useContext(p)},le.useDebugValue=function(){},le.useDeferredValue=function(p,D){return P.H.useDeferredValue(p,D)},le.useEffect=function(p,D){return P.H.useEffect(p,D)},le.useEffectEvent=function(p){return P.H.useEffectEvent(p)},le.useId=function(){return P.H.useId()},le.useImperativeHandle=function(p,D,W){return P.H.useImperativeHandle(p,D,W)},le.useInsertionEffect=function(p,D){return P.H.useInsertionEffect(p,D)},le.useLayoutEffect=function(p,D){return P.H.useLayoutEffect(p,D)},le.useMemo=function(p,D){return P.H.useMemo(p,D)},le.useOptimistic=function(p,D){return P.H.useOptimistic(p,D)},le.useReducer=function(p,D,W){return P.H.useReducer(p,D,W)},le.useRef=function(p){return P.H.useRef(p)},le.useState=function(p){return P.H.useState(p)},le.useSyncExternalStore=function(p,D,W){return P.H.useSyncExternalStore(p,D,W)},le.useTransition=function(){return P.H.useTransition()},le.version="19.3.1",le}var Lf;function nu(){return Lf||(Lf=1,Ic.exports=yh()),Ic.exports}var Q=nu(),Yc={exports:{}},bl={},Qc={exports:{}},Pc={};/**
 * @license React
 * scheduler.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Hf;function hh(){return Hf||(Hf=1,(function(o){function c(H,ne){var Z=H.length;H.push(ne);e:for(;0<Z;){var Ee=Z-1>>>1,he=H[Ee];if(0<j(he,ne))H[Ee]=ne,H[Z]=he,Z=Ee;else break e}}function m(H){return H.length===0?null:H[0]}function d(H){if(H.length===0)return null;var ne=H[0],Z=H.pop();if(Z!==ne){H[0]=Z;e:for(var Ee=0,he=H.length,gt=he>>>1;Ee<gt;){var bt=2*(Ee+1)-1,xt=H[bt],p=bt+1,D=H[p];if(0>j(xt,Z))p<he&&0>j(D,xt)?(H[Ee]=D,H[p]=Z,Ee=p):(H[Ee]=xt,H[bt]=Z,Ee=bt);else if(p<he&&0>j(D,Z))H[Ee]=D,H[p]=Z,Ee=p;else break e}}return ne}function j(H,ne){var Z=H.sortIndex-ne.sortIndex;return Z!==0?Z:H.id-ne.id}if(o.unstable_now=void 0,typeof performance=="object"&&typeof performance.now=="function"){var O=performance;o.unstable_now=function(){return O.now()}}else{var N=Date,L=N.now();o.unstable_now=function(){return N.now()-L}}var B=[],R=[],x=1,g=null,q=3,$=!1,M=!1,X=!1,I=!1,J=typeof setTimeout=="function"?setTimeout:null,ae=typeof clearTimeout=="function"?clearTimeout:null,F=typeof setImmediate<"u"?setImmediate:null;function re(H){for(var ne=m(R);ne!==null;){if(ne.callback===null)d(R);else if(ne.startTime<=H)d(R),ne.sortIndex=ne.expirationTime,c(B,ne);else break;ne=m(R)}}function xe(H){if(X=!1,re(H),!M)if(m(B)!==null)M=!0,C||(C=!0,Ne());else{var ne=m(R);ne!==null&&Le(xe,ne.startTime-H)}}var C=!1,z=-1,P=5,ye=-1;function Re(){return I?!0:!(o.unstable_now()-ye<P)}function Me(){if(I=!1,C){var H=o.unstable_now();ye=H;var ne=!0;try{e:{M=!1,X&&(X=!1,ae(z),z=-1),$=!0;var Z=q;try{t:{for(re(H),g=m(B);g!==null&&!(g.expirationTime>H&&Re());){var Ee=g.callback;if(typeof Ee=="function"){g.callback=null,q=g.priorityLevel;var he=Ee(g.expirationTime<=H);if(H=o.unstable_now(),typeof he=="function"){g.callback=he,re(H),ne=!0;break t}g===m(B)&&d(B),re(H)}else d(B);g=m(B)}if(g!==null)ne=!0;else{var gt=m(R);gt!==null&&Le(xe,gt.startTime-H),ne=!1}}break e}finally{g=null,q=Z,$=!1}ne=void 0}}finally{ne?Ne():C=!1}}}var Ne;if(typeof F=="function")Ne=function(){F(Me)};else if(typeof MessageChannel<"u"){var $e=new MessageChannel,it=$e.port2;$e.port1.onmessage=Me,Ne=function(){it.postMessage(null)}}else Ne=function(){J(Me,0)};function Le(H,ne){z=J(function(){H(o.unstable_now())},ne)}o.unstable_IdlePriority=5,o.unstable_ImmediatePriority=1,o.unstable_LowPriority=4,o.unstable_NormalPriority=3,o.unstable_Profiling=null,o.unstable_UserBlockingPriority=2,o.unstable_cancelCallback=function(H){H.callback=null},o.unstable_forceFrameRate=function(H){0>H||125<H?console.error("forceFrameRate takes a positive int between 0 and 125, forcing frame rates higher than 125 fps is not supported"):P=0<H?Math.floor(1e3/H):5},o.unstable_getCurrentPriorityLevel=function(){return q},o.unstable_next=function(H){switch(q){case 1:case 2:case 3:var ne=3;break;default:ne=q}var Z=q;q=ne;try{return H()}finally{q=Z}},o.unstable_requestPaint=function(){I=!0},o.unstable_runWithPriority=function(H,ne){switch(H){case 1:case 2:case 3:case 4:case 5:break;default:H=3}var Z=q;q=H;try{return ne()}finally{q=Z}},o.unstable_scheduleCallback=function(H,ne,Z){var Ee=o.unstable_now();switch(typeof Z=="object"&&Z!==null?(Z=Z.delay,Z=typeof Z=="number"&&0<Z?Ee+Z:Ee):Z=Ee,H){case 1:var he=-1;break;case 2:he=250;break;case 5:he=1073741823;break;case 4:he=1e4;break;default:he=5e3}return he=Z+he,H={id:x++,callback:ne,priorityLevel:H,startTime:Z,expirationTime:he,sortIndex:-1},Z>Ee?(H.sortIndex=Z,c(R,H),m(B)===null&&H===m(R)&&(X?(ae(z),z=-1):X=!0,Le(xe,Z-Ee))):(H.sortIndex=he,c(B,H),M||$||(M=!0,C||(C=!0,Ne()))),H},o.unstable_shouldYield=Re,o.unstable_wrapCallback=function(H){var ne=q;return function(){var Z=q;q=ne;try{return H.apply(this,arguments)}finally{q=Z}}}})(Pc)),Pc}var Gf;function gh(){return Gf||(Gf=1,Qc.exports=hh()),Qc.exports}var Kc={exports:{}},mt={};/**
 * @license React
 * react-dom.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var If;function bh(){if(If)return mt;If=1;var o=nu();function c(x){var g="https://react.dev/errors/"+x;if(1<arguments.length){g+="?args[]="+encodeURIComponent(arguments[1]);for(var q=2;q<arguments.length;q++)g+="&args[]="+encodeURIComponent(arguments[q])}return"Minified React error #"+x+"; visit "+g+" for the full message or use the non-minified dev environment for full errors and additional helpful warnings."}function m(){}var d={d:{f:m,r:function(){throw Error(c(522))},D:m,C:m,L:m,m,X:m,S:m,M:m},p:0,findDOMNode:null},j=Symbol.for("react.portal"),O=Symbol.for("react.recoverable"),N=Symbol.for("react.optimistic_key");function L(x,g,q){var $=3<arguments.length&&arguments[3]!==void 0?arguments[3]:null;return{$$typeof:j,key:$==null?null:$===N?N:""+$,children:x,containerInfo:g,implementation:q}}var B=o.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE;function R(x,g){if(x==="font")return"";if(typeof g=="string")return g==="use-credentials"?g:""}return mt.__DOM_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE=d,mt.browser=function(x){return{$$typeof:O,_reason:x}},mt.createPortal=function(x,g){var q=2<arguments.length&&arguments[2]!==void 0?arguments[2]:null;if(!g||g.nodeType!==1&&g.nodeType!==9&&g.nodeType!==11)throw Error(c(299));return L(x,g,null,q)},mt.flushSync=function(x){var g=B.T,q=d.p;try{if(B.T=null,d.p=2,x)return x()}finally{B.T=g,d.p=q,d.d.f()}},mt.preconnect=function(x,g){typeof x=="string"&&(g?(g=g.crossOrigin,g=typeof g=="string"?g==="use-credentials"?g:"":void 0):g=null,d.d.C(x,g))},mt.prefetchDNS=function(x){typeof x=="string"&&d.d.D(x)},mt.preinit=function(x,g){if(typeof x=="string"&&g&&typeof g.as=="string"){var q=g.as,$=R(q,g.crossOrigin),M=typeof g.integrity=="string"?g.integrity:void 0,X=typeof g.fetchPriority=="string"?g.fetchPriority:void 0;q==="style"?d.d.S(x,typeof g.precedence=="string"?g.precedence:void 0,{crossOrigin:$,integrity:M,fetchPriority:X}):q==="script"&&d.d.X(x,{crossOrigin:$,integrity:M,fetchPriority:X,nonce:typeof g.nonce=="string"?g.nonce:void 0})}},mt.preinitModule=function(x,g){if(typeof x=="string")if(typeof g=="object"&&g!==null){if(g.as==null||g.as==="script"){var q=R(g.as,g.crossOrigin);d.d.M(x,{crossOrigin:q,integrity:typeof g.integrity=="string"?g.integrity:void 0,nonce:typeof g.nonce=="string"?g.nonce:void 0,fetchPriority:typeof g.fetchPriority=="string"?g.fetchPriority:void 0})}}else g==null&&d.d.M(x)},mt.preload=function(x,g){if(typeof x=="string"&&typeof g=="object"&&g!==null&&typeof g.as=="string"){var q=g.as,$=R(q,g.crossOrigin);d.d.L(x,q,{crossOrigin:$,integrity:typeof g.integrity=="string"?g.integrity:void 0,nonce:typeof g.nonce=="string"?g.nonce:void 0,type:typeof g.type=="string"?g.type:void 0,fetchPriority:typeof g.fetchPriority=="string"?g.fetchPriority:void 0,referrerPolicy:typeof g.referrerPolicy=="string"?g.referrerPolicy:void 0,imageSrcSet:typeof g.imageSrcSet=="string"?g.imageSrcSet:void 0,imageSizes:typeof g.imageSizes=="string"?g.imageSizes:void 0,media:typeof g.media=="string"?g.media:void 0})}},mt.preloadModule=function(x,g){if(typeof x=="string")if(g){var q=R(g.as,g.crossOrigin);d.d.m(x,{as:typeof g.as=="string"&&g.as!=="script"?g.as:void 0,crossOrigin:q,integrity:typeof g.integrity=="string"?g.integrity:void 0,nonce:typeof g.nonce=="string"?g.nonce:void 0,fetchPriority:typeof g.fetchPriority=="string"?g.fetchPriority:void 0})}else d.d.m(x)},mt.requestFormReset=function(x){d.d.r(x)},mt.unstable_batchedUpdates=function(x,g){return x(g)},mt.useFormState=function(x,g,q){return B.H.useFormState(x,g,q)},mt.useFormStatus=function(){return B.H.useHostTransitionStatus()},mt.version="19.3.1",mt}var Yf;function xh(){if(Yf)return Kc.exports;Yf=1;function o(){if(!(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__>"u"||typeof __REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE!="function"))try{__REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE(o)}catch(c){console.error(c)}}return o(),Kc.exports=bh(),Kc.exports}/**
 * @license React
 * react-dom-client.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Qf;function wh(){if(Qf)return bl;Qf=1;var o=gh(),c=nu(),m=xh();function d(e){var t="https://react.dev/errors/"+e;if(1<arguments.length){t+="?args[]="+encodeURIComponent(arguments[1]);for(var n=2;n<arguments.length;n++)t+="&args[]="+encodeURIComponent(arguments[n])}return"Minified React error #"+e+"; visit "+t+" for the full message or use the non-minified dev environment for full errors and additional helpful warnings."}function j(e){return!(!e||e.nodeType!==1&&e.nodeType!==9&&e.nodeType!==11)}function O(e){for(var t=e,n=t;n&&!n.alternate;)t=n,(t.flags&4098)!==0&&(e=t.return),n=t.return;for(;t.return;)t=t.return;return t.tag===3?e:null}function N(e){if(e.tag===13){var t=e.memoizedState;if(t===null&&(e=e.alternate,e!==null&&(t=e.memoizedState)),t!==null)return t.dehydrated}return null}function L(e){if(e.tag===31){var t=e.memoizedState;if(t===null&&(e=e.alternate,e!==null&&(t=e.memoizedState)),t!==null)return t.dehydrated}return null}function B(e){if(O(e)!==e)throw Error(d(188))}function R(e){var t=e.alternate;if(!t){if(t=O(e),t===null)throw Error(d(188));return t!==e?null:e}for(var n=e,a=t;;){var i=n.return;if(i===null)break;var l=i.alternate;if(l===null){if(a=i.return,a!==null){n=a;continue}break}if(i.child===l.child){for(l=i.child;l;){if(l===n)return B(i),e;if(l===a)return B(i),t;l=l.sibling}throw Error(d(188))}if(n.return!==a.return)n=i,a=l;else{for(var s=!1,u=i.child;u;){if(u===n){s=!0,n=i,a=l;break}if(u===a){s=!0,a=i,n=l;break}u=u.sibling}if(!s){for(u=l.child;u;){if(u===n){s=!0,n=l,a=i;break}if(u===a){s=!0,a=l,n=i;break}u=u.sibling}if(!s)throw Error(d(189))}}if(n.alternate!==a)throw Error(d(190))}if(n.tag!==3)throw Error(d(188));return n.stateNode.current===n?e:t}function x(e){var t=e.tag;if(t===5||t===26||t===27||t===6)return e;for(e=e.child;e!==null;){if(t=x(e),t!==null)return t;e=e.sibling}return null}function g(e,t,n,a,i,l){for(;e!==null;){if((e.tag===5||e.tag===27||e.tag===6)&&n(e,a,i,l)||(e.tag!==22||e.memoizedState===null)&&(t||e.tag!==5&&e.tag!==27)&&g(e.child,t,n,a,i,l))return!0;e=e.sibling}return!1}function q(e){for(e=e.return;e!==null;){if(e.tag===3||e.tag===5||e.tag===27)return e;e=e.return}return null}function $(e){var t=!1;for(e=e.return;e!==null&&(e.tag===4&&(t=!0),!(e.tag===3||e.tag===5||e.tag===27));)e=e.return;return t}function M(e){var t=[null,null],n=q(e);return n===null||X(t,e,n.child,{foundSelf:!1}),t}function X(e,t,n,a){for(;n!==null;){if(n===t)a.foundSelf=!0;else if(n.tag===5||n.tag===27||n.tag===6){if(a.foundSelf)return e[1]=n,!0;e[0]=n}else if((n.tag!==22||n.memoizedState===null)&&X(e,t,n.child,a))return!0;n=n.sibling}return!1}function I(e){switch(e.tag){case 5:case 27:case 6:return e.stateNode;case 3:return e.stateNode.containerInfo;default:throw Error(d(559))}}var J=null,ae=null;function F(e,t,n){return e===n?!0:e===t?(J=e,!0):!1}function re(e,t,n){return e===n?(ae=e,!1):e===t?(ae!==null&&(J=e),!0):!1}function xe(e){if(e===null)return null;do e=e===null?null:e.return;while(e&&e.tag!==5&&e.tag!==27&&e.tag!==3);return e||null}function C(e,t,n){for(var a=0,i=e;i;i=n(i))a++;i=0;for(var l=t;l;l=n(l))i++;for(;0<a-i;)e=n(e),a--;for(;0<i-a;)t=n(t),i--;for(;a--;){if(e===t||t!==null&&e===t.alternate)return e;e=n(e),t=n(t)}return null}var z=Object.assign,P=Symbol.for("react.element"),ye=Symbol.for("react.transitional.element"),Re=Symbol.for("react.portal"),Me=Symbol.for("react.fragment"),Ne=Symbol.for("react.strict_mode"),$e=Symbol.for("react.profiler"),it=Symbol.for("react.consumer"),Le=Symbol.for("react.context"),H=Symbol.for("react.forward_ref"),ne=Symbol.for("react.suspense"),Z=Symbol.for("react.suspense_list"),Ee=Symbol.for("react.memo"),he=Symbol.for("react.lazy"),gt=Symbol.for("react.activity"),bt=Symbol.for("react.legacy_hidden"),xt=Symbol.for("react.memo_cache_sentinel"),p=Symbol.for("react.view_transition"),D=Symbol.for("react.recoverable"),W=Symbol.iterator;function ee(e){return e===null||typeof e!="object"?null:(e=W&&e[W]||e["@@iterator"],typeof e=="function"?e:null)}var pe=Symbol.for("react.client.reference");function ve(e){if(e==null)return null;if(typeof e=="function")return e.$$typeof===pe?null:e.displayName||e.name||null;if(typeof e=="string")return e;switch(e){case Me:return"Fragment";case $e:return"Profiler";case Ne:return"StrictMode";case ne:return"Suspense";case Z:return"SuspenseList";case gt:return"Activity";case p:return"ViewTransition"}if(typeof e=="object")switch(e.$$typeof){case Re:return"Portal";case Le:return e.displayName||"Context";case it:return(e._context.displayName||"Context")+".Consumer";case H:var t=e.render;return e=e.displayName,e||(e=t.displayName||t.name||"",e=e!==""?"ForwardRef("+e+")":"ForwardRef"),e;case Ee:return t=e.displayName||null,t!==null?t:ve(e.type)||"Memo";case he:t=e._payload,e=e._init;try{return ve(e(t))}catch{}}return null}var ge=Array.isArray,A=c.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE,U=m.__DOM_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE,E={pending:!1,data:null,method:null,action:null},we=[],Ze=-1;function We(e){return{current:e}}function Te(e){0>Ze||(e.current=we[Ze],we[Ze]=null,Ze--)}function V(e,t){Ze++,we[Ze]=e.current,e.current=t}var Ye=We(null),lt=We(null),rt=We(null),wt=We(null);function nn(e,t){switch(V(rt,t),V(lt,e),V(Ye,null),t.nodeType){case 9:case 11:e=(e=t.documentElement)&&(e=e.namespaceURI)?Qm(e):0;break;default:if(e=t.tagName,t=t.namespaceURI)t=Qm(t),e=Pm(t,e);else switch(e){case"svg":e=1;break;case"math":e=2;break;default:e=0}}Te(Ye),V(Ye,e)}function Oa(){Te(Ye),Te(lt),Te(rt)}function rs(e){var t=e.memoizedState;t!==null&&(hi._currentValue=t.memoizedState,V(wt,e)),t=Ye.current;var n=Pm(t,e.type);t!==n&&(V(lt,e),V(Ye,n))}function jl(e){lt.current===e&&(Te(Ye),Te(lt)),wt.current===e&&(Te(wt),hi._currentValue=E)}var ss,_u;function qn(e){if(ss===void 0)try{throw Error()}catch(n){var t=n.stack.trim().match(/\n( *(at )?)/);ss=t&&t[1]||"",_u=-1<n.stack.indexOf(`
    at`)?" (<anonymous>)":-1<n.stack.indexOf("@")?"@unknown:0:0":""}return`
`+ss+e+_u}var os=!1;function cs(e,t){if(!e||os)return"";os=!0;var n=Error.prepareStackTrace;Error.prepareStackTrace=void 0;try{var a={DetermineComponentFrameRoot:function(){try{if(t){var T=function(){throw Error()};if(Object.defineProperty(T.prototype,"props",{set:function(){throw Error()}}),typeof Reflect=="object"&&Reflect.construct){try{Reflect.construct(T,[])}catch(G){var v=G}Reflect.construct(e,[],T)}else{try{T.call()}catch(G){v=G}T=!1;try{var w=Object.getOwnPropertyDescriptor(e.prototype,"props");Object.defineProperty(e.prototype,"props",{configurable:!0,set:function(){throw Error()}}),T=!0,new e}finally{T&&(w!==void 0?Object.defineProperty(e.prototype,"props",w):delete e.prototype.props)}}}else{try{throw Error()}catch(G){v=G}(T=e())&&typeof T.catch=="function"&&T.catch(function(){})}}catch(G){if(G&&v&&typeof G.stack=="string")return[G.stack,v.stack]}return[null,null]}};a.DetermineComponentFrameRoot.displayName="DetermineComponentFrameRoot";var i=Object.getOwnPropertyDescriptor(a.DetermineComponentFrameRoot,"name");i&&i.configurable&&Object.defineProperty(a.DetermineComponentFrameRoot,"name",{value:"DetermineComponentFrameRoot"});var l=a.DetermineComponentFrameRoot(),s=l[0],u=l[1];if(s&&u){var _=s.split(`
`),h=u.split(`
`);for(i=a=0;a<_.length&&!_[a].includes("DetermineComponentFrameRoot");)a++;for(;i<h.length&&!h[i].includes("DetermineComponentFrameRoot");)i++;if(a===_.length||i===h.length)for(a=_.length-1,i=h.length-1;1<=a&&0<=i&&_[a]!==h[i];)i--;for(;1<=a&&0<=i;a--,i--)if(_[a]!==h[i]){if(a!==1||i!==1)do if(a--,i--,0>i||_[a]!==h[i]){var S=`
`+_[a].replace(" at new "," at ");return e.displayName&&S.includes("<anonymous>")&&(S=S.replace("<anonymous>",e.displayName)),S}while(1<=a&&0<=i);break}}}finally{os=!1,Error.prepareStackTrace=n}return(n=e?e.displayName||e.name:"")?qn(n):""}function yp(e,t){switch(e.tag){case 26:case 27:case 5:return qn(e.type);case 16:return qn("Lazy");case 13:return e.child!==t&&t!==null?qn("Suspense Fallback"):qn("Suspense");case 19:return qn("SuspenseList");case 0:case 15:return cs(e.type,!1);case 11:return cs(e.type.render,!1);case 1:return cs(e.type,!0);case 31:return qn("Activity");case 30:return qn("ViewTransition");default:return""}}function mu(e){try{var t="",n=null;do t+=yp(e,n),n=e,e=e.return;while(e);return t}catch(a){return`
Error generating stack: `+a.message+`
`+a.stack}}var us=Object.prototype.hasOwnProperty,ds=o.unstable_scheduleCallback,_s=o.unstable_cancelCallback,hp=o.unstable_shouldYield,gp=o.unstable_requestPaint,Ct=o.unstable_now,bp=o.unstable_getCurrentPriorityLevel,fu=o.unstable_ImmediatePriority,pu=o.unstable_UserBlockingPriority,El=o.unstable_NormalPriority,xp=o.unstable_LowPriority,vu=o.unstable_IdlePriority,wp=o.log,Np=o.unstable_setDisableYieldValue,Ei=null,At=null;function Rn(e){if(typeof wp=="function"&&Np(e),At&&typeof At.setStrictMode=="function")try{At.setStrictMode(Ei,e)}catch{}}var Ot=Math.clz32?Math.clz32:Ep,Sp=Math.log,jp=Math.LN2;function Ep(e){return e>>>=0,e===0?32:31-(Sp(e)/jp|0)|0}var kl=256,Tl=262144,Cl=4194304;function sa(e){var t=e&42;if(t!==0)return t;switch(e&-e){case 1:return 1;case 2:return 2;case 4:return 4;case 8:return 8;case 16:return 16;case 32:return 32;case 64:return 64;case 128:return 128;case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:return e&-e;case 262144:case 524288:case 1048576:case 2097152:return e&3932160;case 4194304:case 8388608:case 16777216:case 33554432:return e&62914560;case 67108864:return 67108864;case 134217728:return 134217728;case 268435456:return 268435456;case 536870912:return 536870912;case 1073741824:return 0;default:return e}}function Al(e,t,n){var a=e.pendingLanes;if(a===0)return 0;var i=0,l=e.suspendedLanes,s=e.pingedLanes;e=e.warmLanes;var u=a&134217727;return u!==0?(a=u&~l,a!==0?i=sa(a):(s&=u,s!==0?i=sa(s):n||(n=u&~e,n!==0&&(i=sa(n))))):(u=a&~l,u!==0?i=sa(u):s!==0?i=sa(s):n||(n=a&~e,n!==0&&(i=sa(n)))),i===0?0:t!==0&&t!==i&&(t&l)===0&&(l=i&-i,n=t&-t,l>=n||l===32&&(n&4194048)!==0)?t:i}function ki(e,t){return(e.pendingLanes&~(e.suspendedLanes&~e.pingedLanes)&t)===0}function yu(e,t){(t&8)!==0&&(t|=t&32);var n=e.entangledLanes;if(n!==0)for(e=e.entanglements,n&=t;0<n;){var a=31-Ot(n),i=1<<a;t|=e[a],n&=~i}return t}function kp(e,t){switch(e){case 1:case 2:case 4:case 8:case 64:return t+250;case 16:case 32:case 128:case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:case 262144:case 524288:case 1048576:case 2097152:return t+5e3;case 4194304:case 8388608:case 16777216:case 33554432:return-1;case 67108864:case 134217728:case 268435456:case 536870912:case 1073741824:return-1;default:return-1}}function hu(){var e=Cl;return Cl<<=1,(Cl&62914560)===0&&(Cl=4194304),e}function ms(e){for(var t=[],n=0;31>n;n++)t.push(e);return t}function Ti(e,t){e.pendingLanes|=t,t!==268435456&&(e.suspendedLanes=0,e.pingedLanes=0,e.warmLanes=0)}function Tp(e,t,n,a,i,l){var s=e.pendingLanes;e.pendingLanes=n,e.suspendedLanes=0,e.pingedLanes=0,e.warmLanes=0,e.expiredLanes&=n,e.entangledLanes&=n,e.errorRecoveryDisabledLanes&=n,e.shellSuspendCounter=0;var u=e.entanglements,_=e.expirationTimes,h=e.hiddenUpdates;for(n=s&~n;0<n;){var S=31-Ot(n),T=1<<S;u[S]=0,_[S]=-1;var v=h[S];if(v!==null)for(h[S]=null,S=0;S<v.length;S++){var w=v[S];w!==null&&(w.lane&=-536870913)}n&=~T}a!==0&&gu(e,a,0),l!==0&&i===0&&e.tag!==0&&(e.suspendedLanes|=l&~(s&~t))}function gu(e,t,n){e.pendingLanes|=t,e.suspendedLanes&=~t;var a=31-Ot(t);e.entangledLanes|=t,e.entanglements[a]=e.entanglements[a]|1073741824|n&261930}function bu(e,t){var n=e.entangledLanes|=t;for(e=e.entanglements;n;){var a=31-Ot(n),i=1<<a;i&t|e[a]&t&&(e[a]|=t),n&=~i}}function xu(e,t){var n=t&-t;return n=(n&42)!==0?1:fs(n),(n&(e.suspendedLanes|t))!==0?0:n}function fs(e){switch(e){case 2:e=1;break;case 8:e=4;break;case 32:e=16;break;case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:case 262144:case 524288:case 1048576:case 2097152:case 4194304:case 8388608:case 16777216:case 33554432:e=128;break;case 268435456:e=134217728;break;default:e=0}return e}function ps(e){return e&=-e,2<e?8<e?(e&134217727)!==0?32:268435456:8:2}function wu(){var e=U.p;return e!==0?e:(e=window.event,e===void 0?32:Af(e.type))}function Nu(e,t){var n=U.p;try{return U.p=e,t()}finally{U.p=n}}var pn=Math.random().toString(36).slice(2),st="__reactFiber$"+pn,Nt="__reactProps$"+pn,qa="__reactContainer$"+pn,Su="__reactEvents$"+pn,Cp="__reactListeners$"+pn,Ap="__reactHandles$"+pn,ju="__reactResources$"+pn,Ci="__reactMarker$"+pn,Ol="__reactLoad$"+pn;function ql(e){delete e[st],delete e[Nt],delete e[Cp],delete e[Ap]}function oa(e){var t;if(t=e[st])return t;for(var n=e.parentNode;n;){if(t=n[qa]||n[st]){if(n=t.alternate,t.child!==null||n!==null&&n.child!==null)for(e=uf(e);e!==null;){if(n=e[st])return n;e=uf(e)}return t}e=n,n=e.parentNode}return null}function Ra(e){if(e=e[st]||e[qa]){var t=e.tag;if(t===5||t===6||t===13||t===31||t===26||t===27||t===3)return e}return null}function Ai(e){var t=e.tag;if(t===5||t===26||t===27||t===6)return e.stateNode;throw Error(d(33))}function Da(e){var t=e[ju];return t||(t=e[ju]={hoistableStyles:new Map,hoistableScripts:new Map}),t}function et(e){e[Ci]=!0}function Eu(e){e[Ol]=void 0}var ku=new Set,Tu={};function ca(e,t){Ma(e,t),Ma(e+"Capture",t)}function Ma(e,t){for(Tu[e]=t,e=0;e<t.length;e++)ku.add(t[e])}var Op=RegExp("^[:A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD][:A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD\\-.0-9\\u00B7\\u0300-\\u036F\\u203F-\\u2040]*$"),Cu={},Au={};function qp(e){return us.call(Au,e)?!0:us.call(Cu,e)?!1:Op.test(e)?Au[e]=!0:(Cu[e]=!0,!1)}var Se=!1;function Ou(){var e=Se;return Se=!1,e}function Rl(e,t,n){if(qp(t))if(n===null)e.removeAttribute(t);else{switch(typeof n){case"undefined":case"function":case"symbol":e.removeAttribute(t);return;case"boolean":var a=t.toLowerCase().slice(0,5);if(a!=="data-"&&a!=="aria-"){e.removeAttribute(t);return}}e.setAttribute(t,n)}}function Dl(e,t,n){if(n===null)e.removeAttribute(t);else{switch(typeof n){case"undefined":case"function":case"symbol":case"boolean":e.removeAttribute(t);return}e.setAttribute(t,n)}}function vn(e,t,n,a){if(a===null)e.removeAttribute(n);else{switch(typeof a){case"undefined":case"function":case"symbol":case"boolean":e.removeAttribute(n);return}e.setAttributeNS(t,n,a)}}function qt(e){switch(typeof e){case"bigint":case"boolean":case"number":case"string":case"undefined":return e;case"object":return e;default:return""}}function qu(e){var t=e.type;return(e=e.nodeName)&&e.toLowerCase()==="input"&&(t==="checkbox"||t==="radio")}function Rp(e,t,n){var a=Object.getOwnPropertyDescriptor(e.constructor.prototype,t);if(!e.hasOwnProperty(t)&&typeof a<"u"&&typeof a.get=="function"&&typeof a.set=="function"){var i=a.get,l=a.set;return Object.defineProperty(e,t,{configurable:!0,get:function(){return i.call(this)},set:function(s){n=""+s,l.call(this,s)}}),Object.defineProperty(e,t,{enumerable:a.enumerable}),{getValue:function(){return n},setValue:function(s){n=""+s},stopTracking:function(){e._valueTracker=null,delete e[t]}}}}function vs(e){if(!e._valueTracker){var t=qu(e)?"checked":"value";e._valueTracker=Rp(e,t,""+e[t])}}function Ru(e){if(!e)return!1;var t=e._valueTracker;if(!t)return!0;var n=t.getValue(),a="";return e&&(a=qu(e)?e.checked?"true":"false":e.value),e=a,e!==n?(t.setValue(e),!0):!1}var Dp=/[\n"\\]/g;function Lt(e){return e.replace(Dp,function(t){return"\\"+t.charCodeAt(0).toString(16)+" "})}function ys(e,t,n,a,i,l,s,u){e.name="",s!=null&&typeof s!="function"&&typeof s!="symbol"&&typeof s!="boolean"?e.type=s:e.removeAttribute("type"),t!=null?s==="number"?(t===0&&e.value===""||e.value!=t)&&(e.value=""+qt(t)):e.value!==""+qt(t)&&(e.value=""+qt(t)):s!=="submit"&&s!=="reset"||e.removeAttribute("value"),t!=null?s==="number"&&e.value==t?hs(e,qt(e.value)):hs(e,qt(t)):n!=null?hs(e,qt(n)):a!=null&&e.removeAttribute("value"),i==null&&l!=null&&(e.defaultChecked=!!l),i!=null&&(e.checked=i&&typeof i!="function"&&typeof i!="symbol"),u!=null&&typeof u!="function"&&typeof u!="symbol"&&typeof u!="boolean"?e.name=""+qt(u):e.removeAttribute("name")}function Du(e,t,n,a,i,l,s,u){if(l!=null&&typeof l!="function"&&typeof l!="symbol"&&typeof l!="boolean"&&(e.type=l),t!=null||n!=null){if(!(l!=="submit"&&l!=="reset"||t!=null)){vs(e);return}n=n!=null?""+qt(n):"",t=t!=null?""+qt(t):n,u||t===e.value||(e.value=t),e.defaultValue=t}a=a??i,a=typeof a!="function"&&typeof a!="symbol"&&!!a,e.checked=u?e.checked:!!a,e.defaultChecked=!!a,s!=null&&typeof s!="function"&&typeof s!="symbol"&&typeof s!="boolean"&&(e.name=s),vs(e)}function hs(e,t){e.defaultValue!==""+t&&(e.defaultValue=""+t)}function za(e,t,n,a){if(e=e.options,t){t={};for(var i=0;i<n.length;i++)t["$"+n[i]]=!0;for(n=0;n<e.length;n++)i=t.hasOwnProperty("$"+e[n].value),e[n].selected!==i&&(e[n].selected=i),i&&a&&(e[n].defaultSelected=!0)}else{for(n=""+qt(n),t=null,i=0;i<e.length;i++){if(e[i].value===n){e[i].selected=!0,a&&(e[i].defaultSelected=!0);return}t!==null||e[i].disabled||(t=e[i])}t!==null&&(t.selected=!0)}}function Mu(e,t,n){if(t!=null&&(t=""+qt(t),t!==e.value&&(e.value=t),n==null)){e.defaultValue!==t&&(e.defaultValue=t);return}e.defaultValue=n!=null?""+qt(n):""}function zu(e,t,n,a){if(t==null){if(a!=null){if(n!=null)throw Error(d(92));if(ge(a)){if(1<a.length)throw Error(d(93));a=a[0]}n=a}n==null&&(n=""),t=n}n=qt(t),e.defaultValue=n,a=e.textContent,a===n&&a!==""&&a!==null&&(e.value=a),vs(e)}function Ua(e,t){if(t){var n=e.firstChild;if(n&&n===e.lastChild&&n.nodeType===3){n.nodeValue=t;return}}e.textContent=t}var Mp=new Set("animationIterationCount aspectRatio borderImageOutset borderImageSlice borderImageWidth boxFlex boxFlexGroup boxOrdinalGroup columnCount columns flex flexGrow flexPositive flexShrink flexNegative flexOrder gridArea gridRow gridRowEnd gridRowSpan gridRowStart gridColumn gridColumnEnd gridColumnSpan gridColumnStart fontWeight lineClamp lineHeight opacity order orphans scale tabSize widows zIndex zoom fillOpacity floodOpacity stopOpacity strokeDasharray strokeDashoffset strokeMiterlimit strokeOpacity strokeWidth MozAnimationIterationCount MozBoxFlex MozBoxFlexGroup MozLineClamp msAnimationIterationCount msFlex msZoom msFlexGrow msFlexNegative msFlexOrder msFlexPositive msFlexShrink msGridColumn msGridColumnSpan msGridRow msGridRowSpan WebkitAnimationIterationCount WebkitBoxFlex WebKitBoxFlexGroup WebkitBoxOrdinalGroup WebkitColumnCount WebkitColumns WebkitFlex WebkitFlexGrow WebkitFlexPositive WebkitFlexShrink WebkitLineClamp".split(" "));function Uu(e,t,n){var a=t.indexOf("--")===0;n==null||typeof n=="boolean"||n===""?a?e.setProperty(t,""):t==="float"?e.cssFloat="":e[t]="":a?e.setProperty(t,n):typeof n!="number"||n===0||Mp.has(t)?t==="float"?e.cssFloat=n:e[t]=(""+n).trim():e[t]=n+"px"}function Bu(e,t,n){if(t!=null&&typeof t!="object")throw Error(d(62));if(e=e.style,n!=null){for(var a in n)!n.hasOwnProperty(a)||t!=null&&t.hasOwnProperty(a)||(a.indexOf("--")===0?e.setProperty(a,""):a==="float"?e.cssFloat="":e[a]="",Se=!0);for(var i in t)a=t[i],t.hasOwnProperty(i)&&n[i]!==a&&(Uu(e,i,a),Se=!0)}else for(var l in t)t.hasOwnProperty(l)&&Uu(e,l,t[l])}function gs(e){if(e.indexOf("-")===-1)return!1;switch(e){case"annotation-xml":case"color-profile":case"font-face":case"font-face-src":case"font-face-uri":case"font-face-format":case"font-face-name":case"missing-glyph":return!1;default:return!0}}var zp=new Map([["acceptCharset","accept-charset"],["htmlFor","for"],["httpEquiv","http-equiv"],["crossOrigin","crossorigin"],["accentHeight","accent-height"],["alignmentBaseline","alignment-baseline"],["arabicForm","arabic-form"],["baselineShift","baseline-shift"],["capHeight","cap-height"],["clipPath","clip-path"],["clipRule","clip-rule"],["colorInterpolation","color-interpolation"],["colorInterpolationFilters","color-interpolation-filters"],["colorProfile","color-profile"],["colorRendering","color-rendering"],["dominantBaseline","dominant-baseline"],["enableBackground","enable-background"],["fillOpacity","fill-opacity"],["fillRule","fill-rule"],["floodColor","flood-color"],["floodOpacity","flood-opacity"],["fontFamily","font-family"],["fontSize","font-size"],["fontSizeAdjust","font-size-adjust"],["fontStretch","font-stretch"],["fontStyle","font-style"],["fontVariant","font-variant"],["fontWeight","font-weight"],["glyphName","glyph-name"],["glyphOrientationHorizontal","glyph-orientation-horizontal"],["glyphOrientationVertical","glyph-orientation-vertical"],["horizAdvX","horiz-adv-x"],["horizOriginX","horiz-origin-x"],["imageRendering","image-rendering"],["letterSpacing","letter-spacing"],["lightingColor","lighting-color"],["markerEnd","marker-end"],["markerMid","marker-mid"],["markerStart","marker-start"],["maskType","mask-type"],["overlinePosition","overline-position"],["overlineThickness","overline-thickness"],["paintOrder","paint-order"],["panose-1","panose-1"],["pointerEvents","pointer-events"],["renderingIntent","rendering-intent"],["shapeRendering","shape-rendering"],["stopColor","stop-color"],["stopOpacity","stop-opacity"],["strikethroughPosition","strikethrough-position"],["strikethroughThickness","strikethrough-thickness"],["strokeDasharray","stroke-dasharray"],["strokeDashoffset","stroke-dashoffset"],["strokeLinecap","stroke-linecap"],["strokeLinejoin","stroke-linejoin"],["strokeMiterlimit","stroke-miterlimit"],["strokeOpacity","stroke-opacity"],["strokeWidth","stroke-width"],["textAnchor","text-anchor"],["textDecoration","text-decoration"],["textRendering","text-rendering"],["transformOrigin","transform-origin"],["underlinePosition","underline-position"],["underlineThickness","underline-thickness"],["unicodeBidi","unicode-bidi"],["unicodeRange","unicode-range"],["unitsPerEm","units-per-em"],["vAlphabetic","v-alphabetic"],["vHanging","v-hanging"],["vIdeographic","v-ideographic"],["vMathematical","v-mathematical"],["vectorEffect","vector-effect"],["vertAdvY","vert-adv-y"],["vertOriginX","vert-origin-x"],["vertOriginY","vert-origin-y"],["wordSpacing","word-spacing"],["writingMode","writing-mode"],["xmlnsXlink","xmlns:xlink"],["xHeight","x-height"]]),Up=/^[\u0000-\u001F ]*j[\r\n\t]*a[\r\n\t]*v[\r\n\t]*a[\r\n\t]*s[\r\n\t]*c[\r\n\t]*r[\r\n\t]*i[\r\n\t]*p[\r\n\t]*t[\r\n\t]*:/i;function Ml(e){return Up.test(""+e)?"javascript:throw new Error('React has blocked a javascript: URL as a security precaution.')":e}function an(){}var bs=null;function xs(e){return e=e.target||e.srcElement||window,e.correspondingUseElement&&(e=e.correspondingUseElement),e.nodeType===3?e.parentNode:e}var Ba=null,Va=null;function Vu(e){var t=Ra(e);if(t&&(e=t.stateNode)){var n=e[Nt]||null;e:switch(e=t.stateNode,t.type){case"input":if(ys(e,n.value,n.defaultValue,n.defaultValue,n.checked,n.defaultChecked,n.type,n.name),t=n.name,n.type==="radio"&&t!=null){for(n=e;n.parentNode;)n=n.parentNode;for(n=n.querySelectorAll('input[name="'+Lt(""+t)+'"][type="radio"]'),t=0;t<n.length;t++){var a=n[t];if(a!==e&&a.form===e.form){var i=a[Nt]||null;if(!i)throw Error(d(90));ys(a,i.value,i.defaultValue,i.defaultValue,i.checked,i.defaultChecked,i.type,i.name)}}for(t=0;t<n.length;t++)a=n[t],a.form===e.form&&Ru(a)}break e;case"textarea":Mu(e,n.value,n.defaultValue);break e;case"select":t=n.value,t!=null&&za(e,!!n.multiple,t,!1)}}}var ws=!1;function Lu(e,t,n){if(ws)return e(t,n);ws=!0;try{var a=e(t);return a}finally{if(ws=!1,(Ba!==null||Va!==null)&&(Mr(),Ba&&(t=Ba,e=Va,Va=Ba=null,Vu(t),e)))for(t=0;t<e.length;t++)Vu(e[t])}}function Oi(e,t){var n=e.stateNode;if(n===null)return null;var a=n[Nt]||null;if(a===null)return null;n=a[t];e:switch(t){case"onClick":case"onClickCapture":case"onDoubleClick":case"onDoubleClickCapture":case"onMouseDown":case"onMouseDownCapture":case"onMouseMove":case"onMouseMoveCapture":case"onMouseUp":case"onMouseUpCapture":case"onMouseEnter":(a=!a.disabled)||(e=e.type,a=!(e==="button"||e==="input"||e==="select"||e==="textarea")),e=!a;break e;default:e=!1}if(e)return null;if(n&&typeof n!="function")throw Error(d(231,t,typeof n));return n}var yn=!(typeof window>"u"||typeof window.document>"u"||typeof window.document.createElement>"u"),Ns=!1;if(yn)try{var qi={};Object.defineProperty(qi,"passive",{get:function(){Ns=!0}}),window.addEventListener("test",qi,qi),window.removeEventListener("test",qi,qi)}catch{Ns=!1}var Dn=null,Ss=null,zl=null;function Hu(){if(zl)return zl;var e,t=Ss,n=t.length,a,i="value"in Dn?Dn.value:Dn.textContent,l=i.length;for(e=0;e<n&&t[e]===i[e];e++);var s=n-e;for(a=1;a<=s&&t[n-a]===i[l-a];a++);return zl=i.slice(e,1<a?1-a:void 0)}function Ul(e){var t=e.keyCode;return"charCode"in e?(e=e.charCode,e===0&&t===13&&(e=13)):e=t,e===10&&(e=13),32<=e||e===13?e:0}function Bl(){return!0}function Gu(){return!1}function pt(e){function t(n,a,i,l,s){this._reactName=n,this._targetInst=i,this.type=a,this.nativeEvent=l,this.target=s,this.currentTarget=null;for(var u in e)e.hasOwnProperty(u)&&(n=e[u],this[u]=n?n(l):l[u]);return this.isDefaultPrevented=(l.defaultPrevented!=null?l.defaultPrevented:l.returnValue===!1)?Bl:Gu,this.isPropagationStopped=Gu,this}return z(t.prototype,{preventDefault:function(){this.defaultPrevented=!0;var n=this.nativeEvent;n&&(n.preventDefault?n.preventDefault():typeof n.returnValue!="unknown"&&(n.returnValue=!1),this.isDefaultPrevented=Bl)},stopPropagation:function(){var n=this.nativeEvent;n&&(n.stopPropagation?n.stopPropagation():typeof n.cancelBubble!="unknown"&&(n.cancelBubble=!0),this.isPropagationStopped=Bl)},persist:function(){},isPersistent:Bl}),t}var Mn={eventPhase:0,bubbles:0,cancelable:0,timeStamp:function(e){return e.timeStamp||Date.now()},defaultPrevented:0,isTrusted:0},Vl=pt(Mn),Ri=z({},Mn,{view:0,detail:0}),Bp=pt(Ri),js,Es,Di,Ll=z({},Ri,{screenX:0,screenY:0,clientX:0,clientY:0,pageX:0,pageY:0,ctrlKey:0,shiftKey:0,altKey:0,metaKey:0,getModifierState:Ts,button:0,buttons:0,relatedTarget:function(e){return e.relatedTarget===void 0?e.fromElement===e.srcElement?e.toElement:e.fromElement:e.relatedTarget},movementX:function(e){return"movementX"in e?e.movementX:(e!==Di&&(Di&&e.type==="mousemove"?(js=e.screenX-Di.screenX,Es=e.screenY-Di.screenY):Es=js=0,Di=e),js)},movementY:function(e){return"movementY"in e?e.movementY:Es}}),Iu=pt(Ll),Vp=z({},Ll,{dataTransfer:0}),Lp=pt(Vp),Hp=z({},Ri,{relatedTarget:0}),ks=pt(Hp),Gp=z({},Mn,{animationName:0,elapsedTime:0,pseudoElement:0}),Ip=pt(Gp),Yp=z({},Mn,{clipboardData:function(e){return"clipboardData"in e?e.clipboardData:window.clipboardData}}),Qp=pt(Yp),Pp=z({},Mn,{data:0}),Yu=pt(Pp),Kp={Esc:"Escape",Spacebar:" ",Left:"ArrowLeft",Up:"ArrowUp",Right:"ArrowRight",Down:"ArrowDown",Del:"Delete",Win:"OS",Menu:"ContextMenu",Apps:"ContextMenu",Scroll:"ScrollLock",MozPrintableKey:"Unidentified"},Xp={8:"Backspace",9:"Tab",12:"Clear",13:"Enter",16:"Shift",17:"Control",18:"Alt",19:"Pause",20:"CapsLock",27:"Escape",32:" ",33:"PageUp",34:"PageDown",35:"End",36:"Home",37:"ArrowLeft",38:"ArrowUp",39:"ArrowRight",40:"ArrowDown",45:"Insert",46:"Delete",112:"F1",113:"F2",114:"F3",115:"F4",116:"F5",117:"F6",118:"F7",119:"F8",120:"F9",121:"F10",122:"F11",123:"F12",144:"NumLock",145:"ScrollLock",224:"Meta"},Fp={Alt:"altKey",Control:"ctrlKey",Meta:"metaKey",Shift:"shiftKey"};function Zp(e){var t=this.nativeEvent;return t.getModifierState?t.getModifierState(e):(e=Fp[e])?!!t[e]:!1}function Ts(){return Zp}var Jp=z({},Ri,{key:function(e){if(e.key){var t=Kp[e.key]||e.key;if(t!=="Unidentified")return t}return e.type==="keypress"?(e=Ul(e),e===13?"Enter":String.fromCharCode(e)):e.type==="keydown"||e.type==="keyup"?Xp[e.keyCode]||"Unidentified":""},code:0,location:0,ctrlKey:0,shiftKey:0,altKey:0,metaKey:0,repeat:0,locale:0,getModifierState:Ts,charCode:function(e){return e.type==="keypress"?Ul(e):0},keyCode:function(e){return e.type==="keydown"||e.type==="keyup"?e.keyCode:0},which:function(e){return e.type==="keypress"?Ul(e):e.type==="keydown"||e.type==="keyup"?e.keyCode:0}}),$p=pt(Jp),Wp=z({},Ll,{pointerId:0,width:0,height:0,pressure:0,tangentialPressure:0,tiltX:0,tiltY:0,twist:0,pointerType:0,isPrimary:0}),Qu=pt(Wp),ev=z({},Mn,{submitter:0}),tv=pt(ev),nv=z({},Ri,{touches:0,targetTouches:0,changedTouches:0,altKey:0,metaKey:0,ctrlKey:0,shiftKey:0,getModifierState:Ts}),av=pt(nv),iv=z({},Mn,{propertyName:0,elapsedTime:0,pseudoElement:0}),lv=pt(iv),rv=z({},Ll,{deltaX:function(e){return"deltaX"in e?e.deltaX:"wheelDeltaX"in e?-e.wheelDeltaX:0},deltaY:function(e){return"deltaY"in e?e.deltaY:"wheelDeltaY"in e?-e.wheelDeltaY:"wheelDelta"in e?-e.wheelDelta:0},deltaZ:0,deltaMode:0}),sv=pt(rv),ov=z({},Mn,{newState:0,oldState:0,source:0}),cv=pt(ov),uv=[9,13,27,32],Cs=yn&&"CompositionEvent"in window,Mi=null;yn&&"documentMode"in document&&(Mi=document.documentMode);var dv=yn&&"TextEvent"in window&&!Mi,Pu=yn&&(!Cs||Mi&&8<Mi&&11>=Mi),Ku=" ",Xu=!1;function Fu(e,t){switch(e){case"keyup":return uv.indexOf(t.keyCode)!==-1;case"keydown":return t.keyCode!==229;case"keypress":case"mousedown":case"focusout":return!0;default:return!1}}function Zu(e){return e=e.detail,typeof e=="object"&&"data"in e?e.data:null}var La=!1;function _v(e,t){switch(e){case"compositionend":return Zu(t);case"keypress":return t.which!==32?null:(Xu=!0,Ku);case"textInput":return e=t.data,e===Ku&&Xu?null:e;default:return null}}function mv(e,t){if(La)return e==="compositionend"||!Cs&&Fu(e,t)?(e=Hu(),zl=Ss=Dn=null,La=!1,e):null;switch(e){case"paste":return null;case"keypress":if(!(t.ctrlKey||t.altKey||t.metaKey)||t.ctrlKey&&t.altKey){if(t.char&&1<t.char.length)return t.char;if(t.which)return String.fromCharCode(t.which)}return null;case"compositionend":return Pu&&t.locale!=="ko"?null:t.data;default:return null}}var fv={color:!0,date:!0,datetime:!0,"datetime-local":!0,email:!0,month:!0,number:!0,password:!0,range:!0,search:!0,tel:!0,text:!0,time:!0,url:!0,week:!0};function Ju(e){var t=e&&e.nodeName&&e.nodeName.toLowerCase();return t==="input"?!!fv[e.type]:t==="textarea"}function $u(e,t,n,a){Ba?Va?Va.push(a):Va=[a]:Ba=a,t=Hr(t,"onChange"),0<t.length&&(n=new Vl("onChange","change",null,n,a),e.push({event:n,listeners:t}))}var zi=null,Ui=null;function pv(e){Vm(e,0)}function Hl(e){var t=Ai(e);if(Ru(t))return e}function Wu(e,t){if(e==="change")return t}var ed=!1;if(yn){var As;if(yn){var Os="oninput"in document;if(!Os){var td=document.createElement("div");td.setAttribute("oninput","return;"),Os=typeof td.oninput=="function"}As=Os}else As=!1;ed=As&&(!document.documentMode||9<document.documentMode)}function nd(){zi&&(zi.detachEvent("onpropertychange",ad),Ui=zi=null)}function ad(e){if(e.propertyName==="value"&&Hl(Ui)){var t=[];$u(t,Ui,e,xs(e)),Lu(pv,t)}}function vv(e,t,n){e==="focusin"?(nd(),zi=t,Ui=n,zi.attachEvent("onpropertychange",ad)):e==="focusout"&&nd()}function yv(e){if(e==="selectionchange"||e==="keyup"||e==="keydown")return Hl(Ui)}function hv(e,t){if(e==="click")return Hl(t)}function gv(e,t){if(e==="input"||e==="change")return Hl(t)}function bv(e,t){return e===t&&(e!==0||1/e===1/t)||e!==e&&t!==t}var Rt=typeof Object.is=="function"?Object.is:bv;function Bi(e,t){if(Rt(e,t))return!0;if(typeof e!="object"||e===null||typeof t!="object"||t===null)return!1;var n=Object.keys(e),a=Object.keys(t);if(n.length!==a.length)return!1;for(a=0;a<n.length;a++){var i=n[a];if(!us.call(t,i)||!Rt(e[i],t[i]))return!1}return!0}function qs(e){if(e=e||(typeof document<"u"?document:void 0),typeof e>"u")return null;try{return e.activeElement||e.body}catch{return e.body}}function id(e){for(;e&&e.firstChild;)e=e.firstChild;return e}function ld(e,t){var n=id(e);e=0;for(var a;n;){if(n.nodeType===3){if(a=e+n.textContent.length,e<=t&&a>=t)return{node:n,offset:t-e};e=a}e:{for(;n;){if(n.nextSibling){n=n.nextSibling;break e}n=n.parentNode}n=void 0}n=id(n)}}function rd(e,t){return e&&t?e===t?!0:e&&e.nodeType===3?!1:t&&t.nodeType===3?rd(e,t.parentNode):"contains"in e?e.contains(t):e.compareDocumentPosition?!!(e.compareDocumentPosition(t)&16):!1:!1}function sd(e){e=e!=null&&e.ownerDocument!=null&&e.ownerDocument.defaultView!=null?e.ownerDocument.defaultView:window;for(var t=qs(e.document);t instanceof e.HTMLIFrameElement;){try{var n=typeof t.contentWindow.location.href=="string"}catch{n=!1}if(n)e=t.contentWindow;else break;t=qs(e.document)}return t}function Rs(e){var t=e&&e.nodeName&&e.nodeName.toLowerCase();return t&&(t==="input"&&(e.type==="text"||e.type==="search"||e.type==="tel"||e.type==="url"||e.type==="password")||t==="textarea"||e.contentEditable==="true")}var xv=yn&&"documentMode"in document&&11>=document.documentMode,Ha=null,Ds=null,Vi=null,Ms=!1;function od(e,t,n){var a=n.window===n?n.document:n.nodeType===9?n:n.ownerDocument;Ms||Ha==null||Ha!==qs(a)||(a=Ha,"selectionStart"in a&&Rs(a)?a={start:a.selectionStart,end:a.selectionEnd}:(a=(a.ownerDocument&&a.ownerDocument.defaultView||window).getSelection(),a={anchorNode:a.anchorNode,anchorOffset:a.anchorOffset,focusNode:a.focusNode,focusOffset:a.focusOffset}),Vi&&Bi(Vi,a)||(Vi=a,a=Hr(Ds,"onSelect"),0<a.length&&(t=new Vl("onSelect","select",null,t,n),e.push({event:t,listeners:a}),t.target=Ha)))}function ua(e,t){var n={};return n[e.toLowerCase()]=t.toLowerCase(),n["Webkit"+e]="webkit"+t,n["Moz"+e]="moz"+t,n}var Ga={animationend:ua("Animation","AnimationEnd"),animationiteration:ua("Animation","AnimationIteration"),animationstart:ua("Animation","AnimationStart"),transitionrun:ua("Transition","TransitionRun"),transitionstart:ua("Transition","TransitionStart"),transitioncancel:ua("Transition","TransitionCancel"),transitionend:ua("Transition","TransitionEnd")},zs={},cd={};yn&&(cd=document.createElement("div").style,"AnimationEvent"in window||(delete Ga.animationend.animation,delete Ga.animationiteration.animation,delete Ga.animationstart.animation),"TransitionEvent"in window||delete Ga.transitionend.transition);function da(e){if(zs[e])return zs[e];if(!Ga[e])return e;var t=Ga[e],n;for(n in t)if(t.hasOwnProperty(n)&&n in cd)return zs[e]=t[n];return e}var ud=da("animationend"),dd=da("animationiteration"),_d=da("animationstart"),wv=da("transitionrun"),Nv=da("transitionstart"),Sv=da("transitioncancel"),md=da("transitionend"),fd=new Map,Us="abort auxClick beforeToggle cancel canPlay canPlayThrough click close contextMenu copy cut drag dragEnd dragEnter dragExit dragLeave dragOver dragStart drop durationChange emptied encrypted ended error fullscreenChange fullscreenError gotPointerCapture input invalid keyDown keyPress keyUp load loadedData loadedMetadata loadStart lostPointerCapture mouseDown mouseMove mouseOut mouseOver mouseUp paste pause play playing pointerCancel pointerDown pointerMove pointerOut pointerOver pointerUp progress rateChange reset resize seeked seeking stalled submit suspend timeUpdate touchCancel touchEnd touchStart volumeChange scroll toggle touchMove waiting wheel".split(" ");Us.push("scrollEnd");function Zt(e,t){fd.set(e,t),ca(t,[e])}var jv=0;function hn(e,t){if(e.name!=null&&e.name!=="auto")return e.name;if(t.autoName!==null)return t.autoName;e=en.identifierPrefix;var n=jv++;return e="_"+e+"t_"+n.toString(32)+"_",t.autoName=e}function pd(e){if(e==null||typeof e=="string")return e;var t=null,n=oi;if(n!==null)for(var a=0;a<n.length;a++){var i=e[n[a]];if(i!=null){if(i==="none")return"none";t=t==null?i:t+(" "+i)}}return t??e.default}function gn(e,t){return e=pd(e),t=pd(t),t==null?e==="auto"?null:e:t==="auto"?null:t}var Gl=typeof reportError=="function"?reportError:function(e){if(typeof window=="object"&&typeof window.ErrorEvent=="function"){var t=new window.ErrorEvent("error",{bubbles:!0,cancelable:!0,message:typeof e=="object"&&e!==null&&typeof e.message=="string"?String(e.message):String(e),error:e});if(!window.dispatchEvent(t))return}else if(typeof process=="object"&&typeof process.emit=="function"){process.emit("uncaughtException",e);return}console.error(e)},Ht=[],Ia=0,Bs=0;function Il(){for(var e=Ia,t=Bs=Ia=0;t<e;){var n=Ht[t];Ht[t++]=null;var a=Ht[t];Ht[t++]=null;var i=Ht[t];Ht[t++]=null;var l=Ht[t];if(Ht[t++]=null,a!==null&&i!==null){var s=a.pending;s===null?i.next=i:(i.next=s.next,s.next=i),a.pending=i}l!==0&&vd(n,i,l)}}function Yl(e,t,n,a){Ht[Ia++]=e,Ht[Ia++]=t,Ht[Ia++]=n,Ht[Ia++]=a,Bs|=a,e.lanes|=a,e=e.alternate,e!==null&&(e.lanes|=a)}function Vs(e,t,n,a){return Yl(e,t,n,a),Ql(e)}function _a(e,t){return Yl(e,null,null,t),Ql(e)}function vd(e,t,n){e.lanes|=n;var a=e.alternate;a!==null&&(a.lanes|=n);for(var i=!1,l=e.return;l!==null;)l.childLanes|=n,a=l.alternate,a!==null&&(a.childLanes|=n),l.tag===22&&(e=l.stateNode,e===null||e._visibility&1||(i=!0)),e=l,l=l.return;return e.tag===3?(l=e.stateNode,i&&t!==null&&(i=31-Ot(n),e=l.hiddenUpdates,a=e[i],a===null?e[i]=[t]:a.push(t),t.lane=n|536870912),l):null}function Ql(e){if(50<sl)throw sl=0,Dr=null,Error(d(185));for(var t=e.return;t!==null;)e=t,t=e.return;return e.tag===3?e.stateNode:null}var Ya={};function Ev(e,t,n,a){this.tag=e,this.key=n,this.sibling=this.child=this.return=this.stateNode=this.type=this.elementType=null,this.index=0,this.refCleanup=this.ref=null,this.pendingProps=t,this.dependencies=this.memoizedState=this.updateQueue=this.memoizedProps=null,this.mode=a,this.subtreeFlags=this.flags=0,this.deletions=null,this.childLanes=this.lanes=0,this.alternate=null}function St(e,t,n,a){return new Ev(e,t,n,a)}function Ls(e){return e=e.prototype,!(!e||!e.isReactComponent)}function bn(e,t){var n=e.alternate;return n===null?(n=St(e.tag,t,e.key,e.mode),n.elementType=e.elementType,n.type=e.type,n.stateNode=e.stateNode,n.alternate=e,e.alternate=n):(n.pendingProps=t,n.type=e.type,n.flags=0,n.subtreeFlags=0,n.deletions=null),n.flags=e.flags&1206910976,n.childLanes=e.childLanes,n.lanes=e.lanes,n.child=e.child,n.memoizedProps=e.memoizedProps,n.memoizedState=e.memoizedState,n.updateQueue=e.updateQueue,t=e.dependencies,n.dependencies=t===null?null:{lanes:t.lanes,firstContext:t.firstContext},n.sibling=e.sibling,n.index=e.index,n.ref=e.ref,n.refCleanup=e.refCleanup,n}function yd(e,t){e.flags&=1206910978;var n=e.alternate;return n===null?(e.childLanes=0,e.lanes=t,e.child=null,e.subtreeFlags=0,e.memoizedProps=null,e.memoizedState=null,e.updateQueue=null,e.dependencies=null,e.stateNode=null):(e.childLanes=n.childLanes,e.lanes=n.lanes,e.child=n.child,e.subtreeFlags=0,e.deletions=null,e.memoizedProps=n.memoizedProps,e.memoizedState=n.memoizedState,e.updateQueue=n.updateQueue,e.type=n.type,t=n.dependencies,e.dependencies=t===null?null:{lanes:t.lanes,firstContext:t.firstContext}),e}function Pl(e,t,n,a,i,l){var s=0;if(a=e,typeof a=="function")Ls(a)&&(s=1);else if(typeof a=="string")s=eh(e,n,Ye.current)?26:e==="html"||e==="head"||e==="body"?27:5;else e:switch(a){case gt:return e=St(31,n,t,i),e.elementType=gt,e.lanes=l,e;case Me:return ma(n.children,i,l,t);case Ne:s=8,i|=24;break;case $e:return e=St(12,n,t,i|2),e.elementType=$e,e.lanes=l,e;case ne:return e=St(13,n,t,i),e.elementType=ne,e.lanes=l,e;case Z:return e=St(19,n,t,i),e.elementType=Z,e.lanes=l,e;case bt:case p:return e=i|32,e=St(30,n,t,e),e.elementType=p,e.lanes=l,e.stateNode={autoName:null,paired:null,clones:null,ref:null},e;default:if(typeof a=="object"&&a!==null)switch(a.$$typeof){case Le:s=10;break e;case it:s=9;break e;case H:s=11;break e;case Ee:s=14;break e;case he:s=16,a=null;break e}s=29,n=Error(d(130,e===null?"null":typeof e,"")),a=null}return t=St(s,n,t,i),t.elementType=e,t.type=a,t.lanes=l,t}function ma(e,t,n,a){return e=St(7,e,a,t),e.lanes=n,e}function Hs(e,t,n){return e=St(6,e,null,t),e.lanes=n,e}function hd(e){var t=St(18,null,null,0);return t.stateNode=e,t}function Gs(e,t,n){return t=St(4,e.children!==null?e.children:[],e.key,t),t.lanes=n,t.stateNode={containerInfo:e.containerInfo,pendingChildren:null,implementation:e.implementation},t}var gd=new WeakMap;function Gt(e,t){if(typeof e=="object"&&e!==null){var n=gd.get(e);return n!==void 0?n:(t={value:e,source:t,stack:mu(t)},gd.set(e,t),t)}return{value:e,source:t,stack:mu(t)}}var Qa=[],Pa=0,Kl=null,Li=0,It=[],Yt=0,zn=null,ln=1,rn="";function xn(e,t){Qa[Pa++]=Li,Qa[Pa++]=Kl,Kl=e,Li=t}function bd(e,t,n){It[Yt++]=ln,It[Yt++]=rn,It[Yt++]=zn,zn=e;var a=ln;e=rn;var i=32-Ot(a)-1;a&=~(1<<i),n+=1;var l=32-Ot(t)+i;if(30<l){var s=i-i%5;l=(a&(1<<s)-1).toString(32),a>>=s,i-=s,ln=1<<32-Ot(t)+i|n<<i|a,rn=l+e}else ln=1<<l|n<<i|a,rn=e}function Xl(e){e.return!==null&&(xn(e,1),bd(e,1,0))}function Is(e){for(;e===Kl;)Kl=Qa[--Pa],Qa[Pa]=null,Li=Qa[--Pa],Qa[Pa]=null;for(;e===zn;)zn=It[--Yt],It[Yt]=null,rn=It[--Yt],It[Yt]=null,ln=It[--Yt],It[Yt]=null}function xd(e,t){It[Yt++]=ln,It[Yt++]=rn,It[Yt++]=zn,ln=t.id,rn=t.overflow,zn=e}var tt=null,Ue=null,ue=!1,Un=null,Qt=!1,Ys=Error(d(519));function Bn(e){var t=Error(d(418,1<arguments.length&&arguments[1]!==void 0&&arguments[1]?"text":"HTML",""));throw Hi(Gt(t,e)),Ys}function wd(e){var t=e.stateNode,n=e.type,a=e.memoizedProps;switch(t[st]=e,t[Nt]=a,n){case"dialog":_e("cancel",t),_e("close",t);break;case"iframe":case"object":case"embed":_e("load",t);break;case"video":case"audio":for(n=0;n<cl.length;n++)_e(cl[n],t);break;case"source":_e("error",t);break;case"img":case"image":case"link":_e("error",t),_e("load",t);break;case"details":_e("toggle",t);break;case"input":_e("invalid",t),Du(t,a.value,a.defaultValue,a.checked,a.defaultChecked,a.type,a.name,!0);break;case"select":_e("invalid",t);break;case"textarea":_e("invalid",t),zu(t,a.value,a.defaultValue,a.children)}n=a.children,typeof n!="string"&&typeof n!="number"&&typeof n!="bigint"||t.textContent===""+n||a.suppressHydrationWarning===!0||Im(t.textContent,n)?(a.popover!=null&&(_e("beforetoggle",t),_e("toggle",t)),a.onScroll!=null&&_e("scroll",t),a.onScrollEnd!=null&&_e("scrollend",t),a.onClick!=null&&(t.onclick=an),t=!0):t=!1,t||Bn(e,!0)}function Fl(e){for(tt=e.return;tt;)switch(tt.tag){case 5:case 31:case 13:Qt=!1;return;case 27:case 3:Qt=!0;return;default:tt=tt.return}}function Ka(e){if(e!==tt)return!1;if(!ue)return Fl(e),ue=!0,!1;var t=e.tag,n;if((n=t!==3&&t!==27)&&((n=t===5)&&(n=e.type,n=!(n!=="form"&&n!=="button")||xc(e.type,e.memoizedProps)),n=!n),n&&Ue&&Bn(e),Fl(e),t===13){if(e=e.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(317));Ue=cf(e)}else if(t===31){if(e=e.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(317));Ue=cf(e)}else t===27?(t=Ue,ea(e.type)?(e=Ac,Ac=null,Ue=e):Ue=t):Ue=tt?Kt(e.stateNode.nextSibling):null;return!0}function fa(){Ue=tt=null,ue=!1}function Qs(){var e=Un;return e!==null&&(kt===null?kt=e:kt.push.apply(kt,e),Un=null),e}function Hi(e){Un===null?Un=[e]:Un.push(e)}var Ps=We(null),pa=null,wn=null;function Vn(e,t,n){V(Ps,t._currentValue),t._currentValue=n}function Nn(e){e._currentValue=Ps.current,Te(Ps)}function Zl(e,t,n){for(;e!==null;){var a=e.alternate;if((e.childLanes&t)!==t?(e.childLanes|=t,a!==null&&(a.childLanes|=t)):a!==null&&(a.childLanes&t)!==t&&(a.childLanes|=t),e===n)break;e=e.return}}function Ks(e,t,n,a){var i=e.child;for(i!==null&&(i.return=e);i!==null;){var l=i.dependencies;if(l!==null){var s=i.child;l=l.firstContext;e:for(;l!==null;){var u=l;l=i;for(var _=0;_<t.length;_++)if(u.context===t[_]){l.lanes|=n,u=l.alternate,u!==null&&(u.lanes|=n),Zl(l.return,n,e),a||(s=null);break e}l=u.next}}else if(i.tag===18){if(s=i.return,s===null)throw Error(d(341));s.lanes|=n,l=s.alternate,l!==null&&(l.lanes|=n),Zl(s,n,e),s=null}else i.tag===13&&i.memoizedState!==null&&i.memoizedState.dehydrated===null?(i.lanes|=n,s=i.alternate,s!==null&&(s.lanes|=n),Zl(i.return,n,e),s=i.child,s=s!==null?s.sibling:null):s=i.child;if(s!==null)s.return=i;else for(s=i;s!==null;){if(s===e){s=null;break}if(i=s.sibling,i!==null){i.return=s.return,s=i;break}s=s.return}i=s}}function va(e,t,n,a){e=null;for(var i=t,l=!1;i!==null;){if(!l){if((i.flags&524288)!==0)l=!0;else if((i.flags&262144)!==0)break}if(i.tag===10){var s=i.alternate;if(s===null)throw Error(d(387));if(s=s.memoizedProps,s!==null){var u=i.type;Rt(i.pendingProps.value,s.value)||(e!==null?e.push(u):e=[u])}}else if(i===wt.current){if(s=i.alternate,s===null)throw Error(d(387));s.memoizedState.memoizedState!==i.memoizedState.memoizedState&&(e!==null?e.push(hi):e=[hi])}i=i.return}return e!==null&&Ks(t,e,n,a),t.flags|=262144,e!==null}function Jl(e){for(e=e.firstContext;e!==null;){if(!Rt(e.context._currentValue,e.memoizedValue))return!0;e=e.next}return!1}function ya(e){pa=e,wn=null,e=e.dependencies,e!==null&&(e.firstContext=null)}function ot(e){return Nd(pa,e)}function $l(e,t){return pa===null&&ya(e),Nd(e,t)}function Nd(e,t){var n=t._currentValue;if(t={context:t,memoizedValue:n,next:null},wn===null){if(e===null)throw Error(d(308));wn=t,e.dependencies={lanes:0,firstContext:t},e.flags|=524288}else wn=wn.next=t;return n}var kv=typeof AbortController<"u"?AbortController:function(){var e=[],t=this.signal={aborted:!1,addEventListener:function(n,a){e.push(a)}};this.abort=function(){t.aborted=!0,e.forEach(function(n){return n()})}},Tv=o.unstable_scheduleCallback,Cv=o.unstable_NormalPriority,Pe={$$typeof:Le,Consumer:null,Provider:null,_currentValue:null,_currentValue2:null,_threadCount:0};function Xs(){return{controller:new kv,data:new Map,refCount:0}}function Gi(e){e.refCount--,e.refCount===0&&Tv(Cv,function(){e.controller.abort()})}function Sd(e,t){if((e.pendingLanes&4194048)!==0){var n=e.transitionTypes;for(n===null&&(n=e.transitionTypes=[]),e=0;e<t.length;e++){var a=t[e];n.indexOf(a)===-1&&n.push(a)}}}var Ii=null;function Av(e){var t=e.transitionTypes;return e.transitionTypes=null,t}var Yi=null,Fs=0,ha=0,Xa=null;function Ov(e,t){if(Yi===null){var n=Yi=[];Fs=0,ha=_c(),Xa={status:"pending",value:void 0,then:function(a){n.push(a)}}}return Fs++,t.then(jd,jd),t}function jd(){if(--Fs===0&&(Ii=null,Yi!==null)){Xa!==null&&(Xa.status="fulfilled");var e=Yi;Yi=null,ha=0,Xa=null;for(var t=0;t<e.length;t++)(0,e[t])()}}function qv(e,t){var n=[],a={status:"pending",value:null,reason:null,then:function(i){n.push(i)}};return e.then(function(){a.status="fulfilled",a.value=t;for(var i=0;i<n.length;i++)(0,n[i])(t)},function(i){for(a.status="rejected",a.reason=i,i=0;i<n.length;i++)(0,n[i])(void 0)}),a}var Ed=A.S;A.S=function(e,t){if(hm=Ct(),typeof t=="object"&&t!==null&&typeof t.then=="function"&&Ov(e,t),Ii!==null)for(var n=_i;n!==null;)Sd(n,Ii),n=n.next;if(n=e.types,n!==null){for(var a=_i;a!==null;)Sd(a,n),a=a.next;if(ha!==0){a=Ii,a===null&&(a=Ii=[]);for(var i=0;i<n.length;i++){var l=n[i];a.indexOf(l)===-1&&a.push(l)}}}Ed!==null&&Ed(e,t)};var ga=We(null);function Zs(){var e=ga.current;return e!==null?e:ze.pooledCache}function Wl(e,t){t===null?V(ga,ga.current):V(ga,t.pool)}function kd(){var e=Zs();return e===null?null:{parent:Pe._currentValue,pool:e}}var Fa=Error(d(460)),Js=Error(d(474)),er=Error(d(542)),tr={then:function(){}};function Td(e){return e=e.status,e==="fulfilled"||e==="rejected"}function Cd(e,t,n){switch(n=e[n],n===void 0?e.push(t):n!==t&&(t.then(an,an),t=n),t.status){case"fulfilled":return t.value;case"rejected":throw e=t.reason,Od(e),e===void 0&&!("reason"in t)?Error(d(600)):e;default:if(typeof t.status=="string")t.then(an,an);else{if(e=ze,e!==null&&100<e.shellSuspendCounter)throw Error(d(482));e=t,e.status="pending",e.then(function(a){if(t.status==="pending"){var i=t;i.status="fulfilled",i.value=a}},function(a){if(t.status==="pending"){var i=t;i.status="rejected",i.reason=a}})}switch(t.status){case"fulfilled":return t.value;case"rejected":throw e=t.reason,Od(e),e}throw xa=t,Fa}}function ba(e){try{var t=e._init;return t(e._payload)}catch(n){throw n!==null&&typeof n=="object"&&typeof n.then=="function"?(xa=n,Fa):n}}var xa=null;function Ad(){if(xa===null)throw Error(d(459));var e=xa;return xa=null,e}function Od(e){if(e===Fa||e===er)throw Error(d(483))}var Za=null,Qi=0;function nr(e){var t=Qi;return Qi+=1,Za===null&&(Za=[]),Cd(Za,e,t)}function Ln(e,t){t=t.props.ref,e.ref=t!==void 0?t:null}function ar(e,t){throw t.$$typeof===P?Error(d(525)):(e=Object.prototype.toString.call(t),Error(d(31,e==="[object Object]"?"object with keys {"+Object.keys(t).join(", ")+"}":e)))}function qd(e){function t(y,f){if(e){var b=y.deletions;b===null?(y.deletions=[f],y.flags|=16):b.push(f)}}function n(y,f){if(!e)return null;for(;f!==null;)t(y,f),f=f.sibling;return null}function a(y){for(var f=new Map;y!==null;)y.key===null?f.set(y.index,y):f.set(y.key,y),y=y.sibling;return f}function i(y,f){return y=bn(y,f),y.index=0,y.sibling=null,y}function l(y,f,b){return y.index=b,e?(b=y.alternate,b!==null?(b=b.index,b<f?(y.flags|=2,f):b):(y.flags|=134217730,f)):(y.flags|=1048576,f)}function s(y){return e&&y.alternate===null&&(y.flags|=134217730),y}function u(y,f,b,k){return f===null||f.tag!==6?(f=Hs(b,y.mode,k),f.return=y,f):(f=i(f,b),f.return=y,f)}function _(y,f,b,k){var Y=b.type;return Y===Me?(y=S(y,f,b.props.children,k,b.key),Ln(y,b),y):f!==null&&(f.elementType===Y||typeof Y=="object"&&Y!==null&&Y.$$typeof===he&&ba(Y)===f.type)?(f=i(f,b.props),Ln(f,b),f.return=y,f):(f=Pl(b.type,b.key,b.props,null,y.mode,k),Ln(f,b),f.return=y,f)}function h(y,f,b,k){return f===null||f.tag!==4||f.stateNode.containerInfo!==b.containerInfo||f.stateNode.implementation!==b.implementation?(f=Gs(b,y.mode,k),f.return=y,f):(f=i(f,b.children||[]),f.return=y,f)}function S(y,f,b,k,Y){return f===null||f.tag!==7?(f=ma(b,y.mode,k,Y),f.return=y,f):(f=i(f,b),f.return=y,f)}function T(y,f,b){if(typeof f=="string"&&f!==""||typeof f=="number"||typeof f=="bigint")return f=Hs(""+f,y.mode,b),f.return=y,f;if(typeof f=="object"&&f!==null){switch(f.$$typeof){case ye:return b=Pl(f.type,f.key,f.props,null,y.mode,b),Ln(b,f),b.return=y,b;case Re:return f=Gs(f,y.mode,b),f.return=y,f;case he:return f=ba(f),T(y,f,b)}if(ge(f)||ee(f))return f=ma(f,y.mode,b,null),f.return=y,f;if(typeof f.then=="function")return T(y,nr(f),b);if(f.$$typeof===Le)return T(y,$l(y,f),b);ar(y,f)}return null}function v(y,f,b,k){var Y=f!==null?f.key:null;if(typeof b=="string"&&b!==""||typeof b=="number"||typeof b=="bigint")return Y!==null?null:u(y,f,""+b,k);if(typeof b=="object"&&b!==null){switch(b.$$typeof){case ye:return b.key===Y?_(y,f,b,k):null;case Re:return b.key===Y?h(y,f,b,k):null;case he:return b=ba(b),v(y,f,b,k)}if(ge(b)||ee(b))return Y!==null?null:S(y,f,b,k,null);if(typeof b.then=="function")return v(y,f,nr(b),k);if(b.$$typeof===Le)return v(y,f,$l(y,b),k);ar(y,b)}return null}function w(y,f,b,k,Y){if(typeof k=="string"&&k!==""||typeof k=="number"||typeof k=="bigint")return y=y.get(b)||null,u(f,y,""+k,Y);if(typeof k=="object"&&k!==null){switch(k.$$typeof){case ye:return y=y.get(k.key===null?b:k.key)||null,_(f,y,k,Y);case Re:return y=y.get(k.key===null?b:k.key)||null,h(f,y,k,Y);case he:return k=ba(k),w(y,f,b,k,Y)}if(ge(k)||ee(k))return y=y.get(b)||null,S(f,y,k,Y,null);if(typeof k.then=="function")return w(y,f,b,nr(k),Y);if(k.$$typeof===Le)return w(y,f,b,$l(f,k),Y);ar(f,k)}return null}function G(y,f,b,k){for(var Y=null,fe=null,te=f,ie=f=0,Fe=null;te!==null&&ie<b.length;ie++){te.index>ie?(Fe=te,te=null):Fe=te.sibling;var be=v(y,te,b[ie],k);if(be===null){te===null&&(te=Fe);break}e&&te&&be.alternate===null&&t(y,te),f=l(be,f,ie),fe===null?Y=be:fe.sibling=be,fe=be,te=Fe}if(ie===b.length)return n(y,te),ue&&xn(y,ie),Y;if(te===null){for(;ie<b.length;ie++)te=T(y,b[ie],k),te!==null&&(f=l(te,f,ie),fe===null?Y=te:fe.sibling=te,fe=te);return ue&&xn(y,ie),Y}for(te=a(te);ie<b.length;ie++)Fe=w(te,y,ie,b[ie],k),Fe!==null&&(e&&(be=Fe.alternate,be!==null&&te.delete(be.key===null?ie:be.key)),f=l(Fe,f,ie),fe===null?Y=Fe:fe.sibling=Fe,fe=Fe);return e&&te.forEach(function(la){return t(y,la)}),ue&&xn(y,ie),Y}function K(y,f,b,k){if(b==null)throw Error(d(151));for(var Y=null,fe=null,te=f,ie=f=0,Fe=null,be=b.next();te!==null&&!be.done;ie++,be=b.next()){te.index>ie?(Fe=te,te=null):Fe=te.sibling;var la=v(y,te,be.value,k);if(la===null){te===null&&(te=Fe);break}e&&te&&la.alternate===null&&t(y,te),f=l(la,f,ie),fe===null?Y=la:fe.sibling=la,fe=la,te=Fe}if(be.done)return n(y,te),ue&&xn(y,ie),Y;if(te===null){for(;!be.done;ie++,be=b.next())be=T(y,be.value,k),be!==null&&(f=l(be,f,ie),fe===null?Y=be:fe.sibling=be,fe=be);return ue&&xn(y,ie),Y}for(te=a(te);!be.done;ie++,be=b.next())be=w(te,y,ie,be.value,k),be!==null&&(e&&(Fe=be.alternate,Fe!==null&&te.delete(Fe.key===null?ie:Fe.key)),f=l(be,f,ie),fe===null?Y=be:fe.sibling=be,fe=be);return e&&te.forEach(function(_h){return t(y,_h)}),ue&&xn(y,ie),Y}function oe(y,f,b,k){if(typeof b=="object"&&b!==null&&b.type===Me&&b.key===null&&b.props.ref===void 0&&(b=b.props.children),typeof b=="object"&&b!==null){switch(b.$$typeof){case ye:e:{for(var Y=b.key;f!==null;){if(f.key===Y){if(Y=b.type,Y===Me){if(f.tag===7){n(y,f.sibling),k=i(f,b.props.children),Ln(k,b),k.return=y,y=k;break e}}else if(f.elementType===Y||typeof Y=="object"&&Y!==null&&Y.$$typeof===he&&ba(Y)===f.type){n(y,f.sibling),k=i(f,b.props),Ln(k,b),k.return=y,y=k;break e}n(y,f);break}else t(y,f);f=f.sibling}b.type===Me?(k=ma(b.props.children,y.mode,k,b.key),Ln(k,b),k.return=y,y=k):(k=Pl(b.type,b.key,b.props,null,y.mode,k),Ln(k,b),k.return=y,y=k)}return s(y);case Re:e:{for(Y=b.key;f!==null;){if(f.key===Y)if(f.tag===4&&f.stateNode.containerInfo===b.containerInfo&&f.stateNode.implementation===b.implementation){n(y,f.sibling),k=i(f,b.children||[]),k.return=y,y=k;break e}else{n(y,f);break}else t(y,f);f=f.sibling}k=Gs(b,y.mode,k),k.return=y,y=k}return s(y);case he:return b=ba(b),oe(y,f,b,k)}if(ge(b))return G(y,f,b,k);if(ee(b)){if(Y=ee(b),typeof Y!="function")throw Error(d(150));return b=Y.call(b),K(y,f,b,k)}if(typeof b.then=="function")return oe(y,f,nr(b),k);if(b.$$typeof===Le)return oe(y,f,$l(y,b),k);ar(y,b)}return typeof b=="string"&&b!==""||typeof b=="number"||typeof b=="bigint"?(b=""+b,f!==null&&f.tag===6?(n(y,f.sibling),k=i(f,b),k.return=y,y=k):(n(y,f),k=Hs(b,y.mode,k),k.return=y,y=k),s(y)):n(y,f)}return function(y,f,b,k){try{Qi=0;var Y=oe(y,f,b,k);return Za=null,Y}catch(te){if(te===Fa||te===er)throw te;var fe=St(29,te,null,y.mode);return fe.lanes=k,fe.return=y,fe}finally{}}}var wa=qd(!0),Rd=qd(!1),Hn=!1;function $s(e){e.updateQueue={baseState:e.memoizedState,firstBaseUpdate:null,lastBaseUpdate:null,shared:{pending:null,lanes:0,hiddenCallbacks:null},callbacks:null}}function Ws(e,t){e=e.updateQueue,t.updateQueue===e&&(t.updateQueue={baseState:e.baseState,firstBaseUpdate:e.firstBaseUpdate,lastBaseUpdate:e.lastBaseUpdate,shared:e.shared,callbacks:null})}function Gn(e){return{lane:e,tag:0,payload:null,callback:null,next:null}}function In(e,t,n){var a=e.updateQueue;if(a===null)return null;if(a=a.shared,(ke&2)!==0){var i=a.pending;return i===null?t.next=t:(t.next=i.next,i.next=t),a.pending=t,t=Ql(e),vd(e,null,n),t}return Yl(e,a,t,n),Ql(e)}function Pi(e,t,n){if(t=t.updateQueue,t!==null&&(t=t.shared,(n&4194048)!==0)){var a=t.lanes;a&=e.pendingLanes,n|=a,t.lanes=n,bu(e,n)}}function eo(e,t){var n=e.updateQueue,a=e.alternate;if(a!==null&&(a=a.updateQueue,n===a)){var i=null,l=null;if(n=n.firstBaseUpdate,n!==null){do{var s={lane:n.lane,tag:n.tag,payload:n.payload,callback:null,next:null};l===null?i=l=s:l=l.next=s,n=n.next}while(n!==null);l===null?i=l=t:l=l.next=t}else i=l=t;n={baseState:a.baseState,firstBaseUpdate:i,lastBaseUpdate:l,shared:a.shared,callbacks:a.callbacks},e.updateQueue=n;return}e=n.lastBaseUpdate,e===null?n.firstBaseUpdate=t:e.next=t,n.lastBaseUpdate=t}var to=!1;function Ki(){if(to){var e=Xa;if(e!==null)throw e}}function Xi(e,t,n,a){to=!1;var i=e.updateQueue;Hn=!1;var l=i.firstBaseUpdate,s=i.lastBaseUpdate,u=i.shared.pending;if(u!==null){i.shared.pending=null;var _=u,h=_.next;_.next=null,s===null?l=h:s.next=h,s=_;var S=e.alternate;S!==null&&(S=S.updateQueue,u=S.lastBaseUpdate,u!==s&&(u===null?S.firstBaseUpdate=h:u.next=h,S.lastBaseUpdate=_))}if(l!==null){var T=i.baseState;s=0,S=h=_=null,u=l;do{var v=u.lane&-536870913,w=v!==u.lane;if(w?(me&v)===v:(a&v)===v){v!==0&&v===ha&&(to=!0),S!==null&&(S=S.next={lane:0,tag:u.tag,payload:u.payload,callback:null,next:null});e:{var G=e,K=u;v=t;var oe=n;switch(K.tag){case 1:if(G=K.payload,typeof G=="function"){T=G.call(oe,T,v);break e}T=G;break e;case 3:G.flags=G.flags&-65537|128;case 0:if(G=K.payload,v=typeof G=="function"?G.call(oe,T,v):G,v==null)break e;T=z({},T,v);break e;case 2:Hn=!0}}v=u.callback,v!==null&&(e.flags|=64,w&&(e.flags|=8192),w=i.callbacks,w===null?i.callbacks=[v]:w.push(v))}else w={lane:v,tag:u.tag,payload:u.payload,callback:u.callback,next:null},S===null?(h=S=w,_=T):S=S.next=w,s|=v;if(u=u.next,u===null){if(u=i.shared.pending,u===null)break;w=u,u=w.next,w.next=null,i.lastBaseUpdate=w,i.shared.pending=null}}while(!0);S===null&&(_=T),i.baseState=_,i.firstBaseUpdate=h,i.lastBaseUpdate=S,l===null&&(i.shared.lanes=0),Zn|=s,e.lanes=s,e.memoizedState=T}}function Dd(e,t){if(typeof e!="function")throw Error(d(191,e));e.call(t)}function Md(e,t){var n=e.callbacks;if(n!==null)for(e.callbacks=null,e=0;e<n.length;e++)Dd(n[e],t)}var Yn=We(null),ir=We(0);function zd(e,t){e=Tn,V(ir,e),V(Yn,t),Tn=e|t.baseLanes}function no(){V(ir,Tn),V(Yn,Yn.current)}function ao(){Tn=ir.current,Te(Yn),Te(ir)}var ct=We(null),ft=null;function Qn(e){var t=e.alternate;V(ut,ut.current&1),V(ct,e),ft===null&&(t===null||Yn.current!==null||t.memoizedState!==null)&&(ft=e)}function io(e){V(ut,ut.current),V(ct,e),ft===null&&(ft=e)}function Ud(e){e.tag===22?(V(ut,ut.current),V(ct,e),ft===null&&(ft=e)):Pn()}function Pn(){V(ut,ut.current),V(ct,ct.current)}function Dt(e){Te(ct),ft===e&&(ft=null),Te(ut)}var ut=We(0);function Fi(e,t){V(ct,ct.current),V(ut,t)}function lo(e){Te(ut),Te(ct),ft===e&&(ft=null)}function lr(e){for(var t=e;t!==null;){if(t.tag===13){var n=t.memoizedState;if(n!==null&&(n=n.dehydrated,n===null||Tc(n)||Cc(n)))return t}else if(t.tag===19&&t.memoizedProps.revealOrder!=="independent"){if((t.flags&128)!==0)return t}else if(t.child!==null){t.child.return=t,t=t.child;continue}if(t===e)break;for(;t.sibling===null;){if(t.return===null||t.return===e)return null;t=t.return}t.sibling.return=t.return,t=t.sibling}return null}var Sn=0,se=null,De=null,Ke=null,rr=!1,Ja=!1,Na=!1,sr=0,Zi=0,$a=null,Rv=0;function Ge(){throw Error(d(321))}function ro(e,t){if(t===null)return!1;for(var n=0;n<t.length&&n<e.length;n++)if(!Rt(e[n],t[n]))return!1;return!0}function so(e,t,n,a,i,l){return Sn=l,se=t,t.memoizedState=null,t.updateQueue=null,t.lanes=0,A.H=e===null||e.memoizedState===null?b_:x_,Na=!1,l=n(a,i),Na=!1,Ja&&(l=Vd(t,n,a,i)),Bd(e),l}function Bd(e){A.H=fr;var t=De!==null&&De.next!==null;if(Sn=0,Ke=De=se=null,rr=!1,Zi=0,$a=null,t)throw Error(d(300));e===null||Xe||(e=e.dependencies,e!==null&&Jl(e)&&(Xe=!0))}function Vd(e,t,n,a){se=e;var i=0;do{if(Ja&&($a=null),Zi=0,Ja=!1,25<=i)throw Error(d(301));if(i+=1,Ke=De=null,e.updateQueue!=null){var l=e.updateQueue;l.lastEffect=null,l.events=null,l.stores=null,l.memoCache!=null&&(l.memoCache.index=0)}A.H=Hv,l=t(n,a)}while(Ja);return l}function Dv(){var e=A.H,t=e.useState()[0];return t=typeof t.then=="function"?Ji(t):t,e=e.useState()[0],(De!==null?De.memoizedState:null)!==e&&(se.flags|=1024),t}function oo(){var e=sr!==0;return sr=0,e}function co(e,t,n){t.updateQueue=e.updateQueue,t.flags&=-2053,e.lanes&=~n}function uo(e){if(rr){for(e=e.memoizedState;e!==null;){var t=e.queue;t!==null&&(t.pending=null),e=e.next}rr=!1}Sn=0,Ke=De=se=null,Ja=!1,Zi=sr=0,$a=null}function vt(){var e={memoizedState:null,baseState:null,baseQueue:null,queue:null,next:null};return Ke===null?se.memoizedState=Ke=e:Ke=Ke.next=e,Ke}function Qe(){if(De===null){var e=se.alternate;e=e!==null?e.memoizedState:null}else e=De.next;var t=Ke===null?se.memoizedState:Ke.next;if(t!==null)Ke=t,De=e;else{if(e===null)throw se.alternate===null?Error(d(467)):Error(d(310));De=e,e={memoizedState:De.memoizedState,baseState:De.baseState,baseQueue:De.baseQueue,queue:De.queue,next:null},Ke===null?se.memoizedState=Ke=e:Ke=Ke.next=e}return Ke}function or(){return{lastEffect:null,events:null,stores:null,memoCache:null}}function Ji(e){var t=Zi;return Zi+=1,$a===null&&($a=[]),e=Cd($a,e,t),t=se,(Ke===null?t.memoizedState:Ke.next)===null&&(t=t.alternate,A.H=t===null||t.memoizedState===null?b_:x_),e}function cr(e){if(e!==null&&typeof e=="object"){if(typeof e.then=="function")return Ji(e);if(e.$$typeof===D)return;if(e.$$typeof===Le)return ot(e)}throw Error(d(438,String(e)))}function _o(e){var t=null,n=se.updateQueue;if(n!==null&&(t=n.memoCache),t==null){var a=se.alternate;a!==null&&(a=a.updateQueue,a!==null&&(a=a.memoCache,a!=null&&(t={data:a.data.map(function(i){return i.slice()}),index:0})))}if(t==null&&(t={data:[],index:0}),n===null&&(n=or(),se.updateQueue=n),n.memoCache=t,n=t.data[t.index],n===void 0)for(n=t.data[t.index]=Array(e),a=0;a<e;a++)n[a]=xt;return t.index++,n}function jn(e,t){return typeof t=="function"?t(e):t}function ur(e){var t=Qe();return mo(t,De,e)}function mo(e,t,n){var a=e.queue;if(a===null)throw Error(d(311));a.lastRenderedReducer=n;var i=e.baseQueue,l=a.pending;if(l!==null){if(i!==null){var s=i.next;i.next=l.next,l.next=s}t.baseQueue=i=l,a.pending=null}if(l=e.baseState,i===null)e.memoizedState=l;else{t=i.next;var u=s=null,_=null,h=t,S=!1;do{var T=h.lane&-536870913;if(T!==h.lane?(me&T)===T:(Sn&T)===T){var v=h.revertLane;if(v===0)_!==null&&(_=_.next={lane:0,revertLane:0,gesture:null,action:h.action,hasEagerState:h.hasEagerState,eagerState:h.eagerState,next:null}),T===ha&&(S=!0);else if((Sn&v)===v){h=h.next,v===ha&&(S=!0);continue}else T={lane:0,revertLane:h.revertLane,gesture:null,action:h.action,hasEagerState:h.hasEagerState,eagerState:h.eagerState,next:null},_===null?(u=_=T,s=l):_=_.next=T,se.lanes|=v,Zn|=v;T=h.action,Na&&n(l,T),l=h.hasEagerState?h.eagerState:n(l,T)}else v={lane:T,revertLane:h.revertLane,gesture:h.gesture,action:h.action,hasEagerState:h.hasEagerState,eagerState:h.eagerState,next:null},_===null?(u=_=v,s=l):_=_.next=v,se.lanes|=T,Zn|=T;h=h.next}while(h!==null&&h!==t);if(_===null?s=l:_.next=u,!Rt(l,e.memoizedState)&&(Xe=!0,S&&(n=Xa,n!==null)))throw n;e.memoizedState=l,e.baseState=s,e.baseQueue=_,a.lastRenderedState=l}return i===null&&(a.lanes=0),[e.memoizedState,a.dispatch]}function fo(e){var t=Qe(),n=t.queue;if(n===null)throw Error(d(311));n.lastRenderedReducer=e;var a=n.dispatch,i=n.pending,l=t.memoizedState;if(i!==null){n.pending=null;var s=i=i.next;do l=e(l,s.action),s=s.next;while(s!==i);Rt(l,t.memoizedState)||(Xe=!0),t.memoizedState=l,t.baseQueue===null&&(t.baseState=l),n.lastRenderedState=l}return[l,a]}function Ld(e,t,n){var a=se,i=Qe(),l=ue;if(l){if(n===void 0)throw Error(d(407));n=n()}else n=t();var s=!Rt((De||i).memoizedState,n);if(s&&(i.memoizedState=n,Xe=!0),i=i.queue,yo(Id.bind(null,a,i,e),[e]),e=i.getSnapshot!==t||s||Ke!==null&&(Ke.memoizedState.tag&1)!==0,Wa(e?9:8,{destroy:void 0},Gd.bind(null,a,i,n,t),null),e){if(a.flags|=2048,ze===null)throw Error(d(349));l||(Sn&127)!==0||Hd(a,t,n)}return n}function Hd(e,t,n){e.flags|=16384,e={getSnapshot:t,value:n},t=se.updateQueue,t===null?(t=or(),se.updateQueue=t,t.stores=[e]):(n=t.stores,n===null?t.stores=[e]:n.push(e))}function Gd(e,t,n,a){t.value=n,t.getSnapshot=a,Yd(t)&&Qd(e)}function Id(e,t,n){return n(function(){Yd(t)&&Qd(e)})}function Yd(e){var t=e.getSnapshot;e=e.value;try{var n=t();return!Rt(e,n)}catch{return!0}}function Qd(e){var t=_a(e,2);t!==null&&Tt(t,e,2)}function po(e){var t=vt();if(typeof e=="function"){var n=e;if(e=n(),Na){Rn(!0);try{n()}finally{Rn(!1)}}}return t.memoizedState=t.baseState=e,t.queue={pending:null,lanes:0,dispatch:null,lastRenderedReducer:jn,lastRenderedState:e},t}function Pd(e,t,n,a){return e.baseState=n,mo(e,De,typeof a=="function"?a:jn)}function Mv(e,t,n,a,i){if(mr(e))throw Error(d(485));if(e=t.action,e!==null){var l={payload:i,action:e,next:null,isTransition:!0,status:"pending",value:null,reason:null,listeners:[],then:function(s){l.listeners.push(s)}};A.T!==null?n(!0):l.isTransition=!1,a(l),n=t.pending,n===null?(l.next=t.pending=l,Kd(t,l)):(l.next=n.next,t.pending=n.next=l)}}function Kd(e,t){var n=t.action,a=t.payload,i=e.state;if(t.isTransition){var l=A.T,s={};s.types=l!==null?l.types:null,A.T=s;try{var u=n(i,a),_=A.S;_!==null&&_(s,u),Xd(e,t,u)}catch(h){vo(e,t,h)}finally{l!==null&&s.types!==null&&(l.types=s.types),A.T=l}}else try{l=n(i,a),Xd(e,t,l)}catch(h){vo(e,t,h)}}function Xd(e,t,n){n!==null&&typeof n=="object"&&typeof n.then=="function"?n.then(function(a){Fd(e,t,a)},function(a){return vo(e,t,a)}):Fd(e,t,n)}function Fd(e,t,n){t.status="fulfilled",t.value=n,Zd(t),e.state=n,t=e.pending,t!==null&&(n=t.next,n===t?e.pending=null:(n=n.next,t.next=n,Kd(e,n)))}function vo(e,t,n){var a=e.pending;if(e.pending=null,a!==null){a=a.next;do t.status="rejected",t.reason=n,Zd(t),t=t.next;while(t!==a)}e.action=null}function Zd(e){e=e.listeners;for(var t=0;t<e.length;t++)(0,e[t])()}function Jd(e,t){return t}function $d(e,t){if(ue){var n=ze.formState;if(n!==null){e:{var a=se;if(ue){if(Ue){t:{for(var i=Ue,l=Qt;i.nodeType!==8;){if(!l){i=null;break t}if(i=Kt(i.nextSibling),i===null){i=null;break t}}l=i.data,i=l==="F!"||l==="F"?i:null}if(i){Ue=Kt(i.nextSibling),a=i.data==="F!";break e}}Bn(a)}a=!1}a&&(t=n[0])}}return n=vt(),n.memoizedState=n.baseState=t,a={pending:null,lanes:0,dispatch:null,lastRenderedReducer:Jd,lastRenderedState:t},n.queue=a,n=y_.bind(null,se,a),a.dispatch=n,a=po(!1),l=wo.bind(null,se,!1,a.queue),a=vt(),i={state:t,dispatch:null,action:e,pending:null},a.queue=i,n=Mv.bind(null,se,i,l,n),i.dispatch=n,a.memoizedState=e,[t,n,!1]}function Wd(e){var t=Qe();return e_(t,De,e)}function e_(e,t,n){if(t=mo(e,t,Jd)[0],e=ur(jn)[0],typeof t=="object"&&t!==null&&typeof t.then=="function")try{var a=Ji(t)}catch(s){throw s===Fa?er:s}else a=t;t=Qe();var i=t.queue,l=i.dispatch;return n!==t.memoizedState&&(se.flags|=2048,Wa(9,{destroy:void 0},zv.bind(null,i,n),null)),[a,l,e]}function zv(e,t){e.action=t}function t_(e){var t=Qe(),n=De;if(n!==null)return e_(t,n,e);Qe(),t=t.memoizedState,n=Qe();var a=n.queue.dispatch;return n.memoizedState=e,[t,a,!1]}function Wa(e,t,n,a){return e={tag:e,create:n,deps:a,inst:t,next:null},t=se.updateQueue,t===null&&(t=or(),se.updateQueue=t),n=t.lastEffect,n===null?t.lastEffect=e.next=e:(a=n.next,n.next=e,e.next=a,t.lastEffect=e),e}function n_(){return Qe().memoizedState}function dr(e,t,n,a){var i=vt();se.flags|=e,i.memoizedState=Wa(1|t,{destroy:void 0},n,a===void 0?null:a)}function _r(e,t,n,a){var i=Qe();a=a===void 0?null:a;var l=i.memoizedState.inst;De!==null&&a!==null&&ro(a,De.memoizedState.deps)?i.memoizedState=Wa(t,l,n,a):(se.flags|=e,i.memoizedState=Wa(1|t,l,n,a))}function a_(e,t){dr(8390656,8,e,t)}function yo(e,t){_r(2048,8,e,t)}function Uv(e){se.flags|=4;var t=se.updateQueue;if(t===null)t=or(),se.updateQueue=t,t.events=[e];else{var n=t.events;n===null?t.events=[e]:n.push(e)}}function i_(e){var t=Qe().memoizedState;return Uv({ref:t,nextImpl:e}),function(){if((ke&2)!==0)throw Error(d(440));return t.impl.apply(void 0,arguments)}}function l_(e,t){return _r(4,2,e,t)}function r_(e,t){return _r(4,4,e,t)}function s_(e,t){if(typeof t=="function"){e=e();var n=t(e);return function(){typeof n=="function"?n():t(null)}}if(t!=null)return e=e(),t.current=e,function(){t.current=null}}function o_(e,t,n){n=n!=null?n.concat([e]):null,_r(4,4,s_.bind(null,t,e),n)}function ho(){}function c_(e,t){var n=Qe();t=t===void 0?null:t;var a=n.memoizedState;return t!==null&&ro(t,a[1])?a[0]:(n.memoizedState=[e,t],e)}function u_(e,t){var n=Qe();t=t===void 0?null:t;var a=n.memoizedState;if(t!==null&&ro(t,a[1]))return a[0];if(a=e(),Na){Rn(!0);try{e()}finally{Rn(!1)}}return n.memoizedState=[a,t],a}function go(e,t,n){return n===void 0||(Sn&1073741824)!==0&&(me&261930)===0?e.memoizedState=t:(e.memoizedState=n,e=bm(),se.lanes|=e,Zn|=e,n)}function d_(e,t,n,a){return Rt(n,t)?n:Yn.current!==null?(e=go(e,n,a),Rt(e,t)||(Xe=!0),e):(Sn&106)===0||(Sn&1073741824)!==0&&(me&261930)===0?(Xe=!0,e.memoizedState=n):(e=bm(),se.lanes|=e,Zn|=e,t)}function __(e,t,n,a,i){var l=U.p;U.p=l!==0&&8>l?l:8;var s=A.T,u={};u.types=s!==null?s.types:null,A.T=u,wo(e,!1,t,n);try{var _=i(),h=A.S;if(h!==null&&h(u,_),_!==null&&typeof _=="object"&&typeof _.then=="function"){var S=qv(_,a);$i(e,t,S,Bt(e))}else $i(e,t,a,Bt(e))}catch(T){$i(e,t,{then:function(){},status:"rejected",reason:T},Bt())}finally{U.p=l,s!==null&&u.types!==null&&(s.types=u.types),A.T=s}}function Bv(){}function bo(e,t,n,a){if(e.tag!==5)throw Error(d(476));var i=m_(e).queue;__(e,i,t,E,n===null?Bv:function(){return f_(e),n(a)})}function m_(e){var t=e.memoizedState;if(t!==null)return t;t={memoizedState:E,baseState:E,baseQueue:null,queue:{pending:null,lanes:0,dispatch:null,lastRenderedReducer:jn,lastRenderedState:E},next:null};var n={};return t.next={memoizedState:n,baseState:n,baseQueue:null,queue:{pending:null,lanes:0,dispatch:null,lastRenderedReducer:jn,lastRenderedState:n},next:null},e.memoizedState=t,e=e.alternate,e!==null&&(e.memoizedState=t),t}function f_(e){var t=m_(e);t.next===null&&(t=e.alternate.memoizedState),$i(e,t.next.queue,{},Bt())}function xo(){return ot(hi)}function p_(){return Qe().memoizedState}function v_(){return Qe().memoizedState}function Vv(e){for(var t=e.return;t!==null;){switch(t.tag){case 24:case 3:var n=Bt();e=Gn(n);var a=In(t,e,n);a!==null&&(Tt(a,t,n),Pi(a,t,n)),t={cache:Xs()},e.payload=t;return}t=t.return}}function Lv(e,t,n){var a=Bt();n={lane:a,revertLane:0,gesture:null,action:n,hasEagerState:!1,eagerState:null,next:null},mr(e)?h_(t,n):(n=Vs(e,t,n,a),n!==null&&(Tt(n,e,a),g_(n,t,a)))}function y_(e,t,n){var a=Bt();$i(e,t,n,a)}function $i(e,t,n,a){var i={lane:a,revertLane:0,gesture:null,action:n,hasEagerState:!1,eagerState:null,next:null};if(mr(e))h_(t,i);else{var l=e.alternate;if(e.lanes===0&&(l===null||l.lanes===0)&&(l=t.lastRenderedReducer,l!==null))try{var s=t.lastRenderedState,u=l(s,n);if(i.hasEagerState=!0,i.eagerState=u,Rt(u,s))return Yl(e,t,i,0),ze===null&&Il(),!1}catch{}finally{}if(n=Vs(e,t,i,a),n!==null)return Tt(n,e,a),g_(n,t,a),!0}return!1}function wo(e,t,n,a){if(a={lane:2,revertLane:_c(),gesture:null,action:a,hasEagerState:!1,eagerState:null,next:null},mr(e)){if(t)throw Error(d(479))}else t=Vs(e,n,a,2),t!==null&&Tt(t,e,2)}function mr(e){var t=e.alternate;return e===se||t!==null&&t===se}function h_(e,t){Ja=rr=!0;var n=e.pending;n===null?t.next=t:(t.next=n.next,n.next=t),e.pending=t}function g_(e,t,n){if((n&4194048)!==0){var a=t.lanes;a&=e.pendingLanes,n|=a,t.lanes=n,bu(e,n)}}var fr={readContext:ot,use:cr,useCallback:Ge,useContext:Ge,useEffect:Ge,useImperativeHandle:Ge,useLayoutEffect:Ge,useInsertionEffect:Ge,useMemo:Ge,useReducer:Ge,useRef:Ge,useState:Ge,useDebugValue:Ge,useDeferredValue:Ge,useTransition:Ge,useSyncExternalStore:Ge,useId:Ge,useHostTransitionStatus:Ge,useFormState:Ge,useActionState:Ge,useOptimistic:Ge,useMemoCache:Ge,useCacheRefresh:Ge,useEffectEvent:Ge},b_={readContext:ot,use:cr,useCallback:function(e,t){return vt().memoizedState=[e,t===void 0?null:t],e},useContext:ot,useEffect:a_,useImperativeHandle:function(e,t,n){n=n!=null?n.concat([e]):null,dr(4194308,4,s_.bind(null,t,e),n)},useLayoutEffect:function(e,t){return dr(4194308,4,e,t)},useInsertionEffect:function(e,t){dr(4,2,e,t)},useMemo:function(e,t){var n=vt();t=t===void 0?null:t;var a=e();if(Na){Rn(!0);try{e()}finally{Rn(!1)}}return n.memoizedState=[a,t],a},useReducer:function(e,t,n){var a=vt();if(n!==void 0){var i=n(t);if(Na){Rn(!0);try{n(t)}finally{Rn(!1)}}}else i=t;return a.memoizedState=a.baseState=i,e={pending:null,lanes:0,dispatch:null,lastRenderedReducer:e,lastRenderedState:i},a.queue=e,e=e.dispatch=Lv.bind(null,se,e),[a.memoizedState,e]},useRef:function(e){var t=vt();return e={current:e},t.memoizedState=e},useState:function(e){e=po(e);var t=e.queue,n=y_.bind(null,se,t);return t.dispatch=n,[e.memoizedState,n]},useDebugValue:ho,useDeferredValue:function(e,t){var n=vt();return go(n,e,t)},useTransition:function(){var e=po(!1);return e=__.bind(null,se,e.queue,!0,!1),vt().memoizedState=e,[!1,e]},useSyncExternalStore:function(e,t,n){var a=se,i=vt();if(ue){if(n===void 0)throw Error(d(407));n=n()}else{if(n=t(),ze===null)throw Error(d(349));(me&127)!==0||Hd(a,t,n)}i.memoizedState=n;var l={value:n,getSnapshot:t};return i.queue=l,a_(Id.bind(null,a,l,e),[e]),a.flags|=2048,Wa(9,{destroy:void 0},Gd.bind(null,a,l,n,t),null),n},useId:function(){var e=vt(),t=ze.identifierPrefix;if(ue){var n=rn,a=ln;n=(a&~(1<<32-Ot(a)-1)).toString(32)+n,t="_"+t+"R_"+n,n=sr++,0<n&&(t+="H"+n.toString(32)),t+="_"}else n=Rv++,t="_"+t+"r_"+n.toString(32)+"_";return e.memoizedState=t},useHostTransitionStatus:xo,useFormState:$d,useActionState:$d,useOptimistic:function(e){var t=vt();t.memoizedState=t.baseState=e;var n={pending:null,lanes:0,dispatch:null,lastRenderedReducer:null,lastRenderedState:null};return t.queue=n,t=wo.bind(null,se,!0,n),n.dispatch=t,[e,t]},useMemoCache:_o,useCacheRefresh:function(){return vt().memoizedState=Vv.bind(null,se)},useEffectEvent:function(e){var t=vt(),n={impl:e};return t.memoizedState=n,function(){if((ke&2)!==0)throw Error(d(440));return n.impl.apply(void 0,arguments)}}},x_={readContext:ot,use:cr,useCallback:c_,useContext:ot,useEffect:yo,useImperativeHandle:o_,useInsertionEffect:l_,useLayoutEffect:r_,useMemo:u_,useReducer:ur,useRef:n_,useState:function(){return ur(jn)},useDebugValue:ho,useDeferredValue:function(e,t){var n=Qe();return d_(n,De.memoizedState,e,t)},useTransition:function(){var e=ur(jn)[0],t=Qe().memoizedState;return[typeof e=="boolean"?e:Ji(e),t]},useSyncExternalStore:Ld,useId:p_,useHostTransitionStatus:xo,useFormState:Wd,useActionState:Wd,useOptimistic:function(e,t){var n=Qe();return Pd(n,De,e,t)},useMemoCache:_o,useCacheRefresh:v_,useEffectEvent:i_},Hv={readContext:ot,use:cr,useCallback:c_,useContext:ot,useEffect:yo,useImperativeHandle:o_,useInsertionEffect:l_,useLayoutEffect:r_,useMemo:u_,useReducer:fo,useRef:n_,useState:function(){return fo(jn)},useDebugValue:ho,useDeferredValue:function(e,t){var n=Qe();return De===null?go(n,e,t):d_(n,De.memoizedState,e,t)},useTransition:function(){var e=fo(jn)[0],t=Qe().memoizedState;return[typeof e=="boolean"?e:Ji(e),t]},useSyncExternalStore:Ld,useId:p_,useHostTransitionStatus:xo,useFormState:t_,useActionState:t_,useOptimistic:function(e,t){var n=Qe();return De!==null?Pd(n,De,e,t):(n.baseState=e,[e,n.queue.dispatch])},useMemoCache:_o,useCacheRefresh:v_,useEffectEvent:i_};function No(e,t,n,a){t=e.memoizedState,n=n(a,t),n=n==null?t:z({},t,n),e.memoizedState=n,e.lanes===0&&(e.updateQueue.baseState=n)}var So={enqueueSetState:function(e,t,n){e=e._reactInternals;var a=Bt(),i=Gn(a);i.payload=t,n!=null&&(i.callback=n),t=In(e,i,a),t!==null&&(Tt(t,e,a),Pi(t,e,a))},enqueueReplaceState:function(e,t,n){e=e._reactInternals;var a=Bt(),i=Gn(a);i.tag=1,i.payload=t,n!=null&&(i.callback=n),t=In(e,i,a),t!==null&&(Tt(t,e,a),Pi(t,e,a))},enqueueForceUpdate:function(e,t){e=e._reactInternals;var n=Bt(),a=Gn(n);a.tag=2,t!=null&&(a.callback=t),t=In(e,a,n),t!==null&&(Tt(t,e,n),Pi(t,e,n))}};function w_(e,t,n,a,i,l,s){return e=e.stateNode,typeof e.shouldComponentUpdate=="function"?e.shouldComponentUpdate(a,l,s):t.prototype&&t.prototype.isPureReactComponent?!Bi(n,a)||!Bi(i,l):!0}function N_(e,t,n,a){e=t.state,typeof t.componentWillReceiveProps=="function"&&t.componentWillReceiveProps(n,a),typeof t.UNSAFE_componentWillReceiveProps=="function"&&t.UNSAFE_componentWillReceiveProps(n,a),t.state!==e&&So.enqueueReplaceState(t,t.state,null)}function Sa(e,t){var n=t;if("ref"in t){n={};for(var a in t)a!=="ref"&&(n[a]=t[a])}if(e=e.defaultProps){n===t&&(n=z({},n));for(var i in e)n[i]===void 0&&(n[i]=e[i])}return n}function S_(e){Gl(e)}function j_(e){console.error(e)}function E_(e){Gl(e)}function pr(e,t){try{var n=e.onUncaughtError;n(t.value,{componentStack:t.stack})}catch(a){setTimeout(function(){throw a})}}function k_(e,t,n){try{var a=e.onCaughtError;a(n.value,{componentStack:n.stack,errorBoundary:t.tag===1?t.stateNode:null})}catch(i){setTimeout(function(){throw i})}}function jo(e,t,n){return n=Gn(n),n.tag=3,n.payload={element:null},n.callback=function(){pr(e,t)},n}function T_(e){return e=Gn(e),e.tag=3,e}function C_(e,t,n,a){var i=n.type.getDerivedStateFromError;if(typeof i=="function"){var l=a.value;e.payload=function(){return i(l)},e.callback=function(){k_(t,n,a)}}var s=n.stateNode;s!==null&&typeof s.componentDidCatch=="function"&&(e.callback=function(){k_(t,n,a),typeof i!="function"&&(Jn===null?Jn=new Set([this]):Jn.add(this));var u=a.stack;this.componentDidCatch(a.value,{componentStack:u!==null?u:""})})}function Gv(e,t,n,a,i){if(n.flags|=32768,a!==null&&typeof a=="object"&&typeof a.then=="function"){if(t=n.alternate,t!==null&&va(t,n,i,!0),n=ct.current,n!==null){switch(n.tag){case 31:case 13:case 19:return ft===null?zr():n.alternate===null&&Ie===0&&(Ie=3),n.flags&=-257,n.flags|=65536,n.lanes=i,a===tr?n.flags|=16384:(t=n.updateQueue,t===null?n.updateQueue=new Set([a]):t.add(a),cc(e,a,i)),!1;case 22:return n.flags|=65536,a===tr?n.flags|=16384:(t=n.updateQueue,t===null?(t={transitions:null,markerInstances:null,retryQueue:new Set([a])},n.updateQueue=t):(n=t.retryQueue,n===null?t.retryQueue=new Set([a]):n.add(a)),cc(e,a,i)),!1}throw Error(d(435,n.tag))}return cc(e,a,i),zr(),!1}if(ue)return t=ct.current,t!==null?((t.flags&65536)===0&&(t.flags|=256),t.flags|=65536,t.lanes=i,a!==Ys&&(e=Error(d(422),{cause:a}),Hi(Gt(e,n)))):(a!==Ys&&(t=Error(d(423),{cause:a}),Hi(Gt(t,n))),e=e.current.alternate,e.flags|=65536,i&=-i,e.lanes|=i,a=Gt(a,n),i=jo(e.stateNode,a,i),eo(e,i),Ie!==4&&(Ie=2)),!1;var l=Error(d(520),{cause:a});if(l=Gt(l,n),rl===null?rl=[l]:rl.push(l),Ie!==4&&(Ie=2),t===null)return!0;a=Gt(a,n),n=t;do{switch(n.tag){case 3:return n.flags|=65536,e=i&-i,n.lanes|=e,e=jo(n.stateNode,a,e),eo(n,e),!1;case 1:if(t=n.type,l=n.stateNode,(n.flags&128)===0&&(typeof t.getDerivedStateFromError=="function"||l!==null&&typeof l.componentDidCatch=="function"&&(Jn===null||!Jn.has(l))))return n.flags|=65536,i&=-i,n.lanes|=i,i=T_(i),C_(i,e,n,a),eo(n,i),!1;break;case 22:if(n.memoizedState!==null)return n.flags|=65536,!1}n=n.return}while(n!==null);return!1}var Eo=Error(d(461)),Xe=!1;function Je(e,t,n,a){t.child=e===null?Rd(t,null,n,a):wa(t,e.child,n,a)}function A_(e,t,n,a,i){n=n.render;var l=t.ref;if("ref"in a){var s={};for(var u in a)u!=="ref"&&(s[u]=a[u])}else s=a;return ya(t),a=so(e,t,n,s,l,i),u=oo(),e!==null&&!Xe?(co(e,t,i),En(e,t,i)):(ue&&u&&Xl(t),t.flags|=1,Je(e,t,a,i),t.child)}function O_(e,t,n,a,i){if(e===null){var l=n.type;return typeof l=="function"&&!Ls(l)&&l.defaultProps===void 0&&n.compare===null?(t.tag=15,t.type=l,q_(e,t,l,a,i)):(e=Pl(n.type,null,a,t,t.mode,i),e.ref=t.ref,e.return=t,t.child=e)}if(l=e.child,!Do(e,i)){var s=l.memoizedProps;if(n=n.compare,n=n!==null?n:Bi,n(s,a)&&e.ref===t.ref)return En(e,t,i)}return t.flags|=1,e=bn(l,a),e.ref=t.ref,e.return=t,t.child=e}function q_(e,t,n,a,i){if(e!==null){var l=e.memoizedProps;if(Bi(l,a)&&e.ref===t.ref)if(Xe=!1,t.pendingProps=a=l,Do(e,i))(e.flags&131072)!==0&&(Xe=!0);else return t.lanes=e.lanes,En(e,t,i)}return ko(e,t,n,a,i)}function R_(e,t,n,a){var i=a.children,l=e!==null?e.memoizedState:null;if(e===null&&t.stateNode===null&&(t.stateNode={_visibility:1,_pendingMarkers:null,_retryCache:null,_transitions:null}),a.mode==="hidden"){if((t.flags&128)!==0){if(l=l!==null?l.baseLanes|n:n,e!==null){for(a=t.child=e.child,i=0;a!==null;)i=i|a.lanes|a.childLanes,a=a.sibling;a=i&~l}else a=0,t.child=null;return D_(e,t,l,n,a)}if((n&536870912)!==0)t.memoizedState={baseLanes:0,cachePool:null},e!==null&&Wl(t,l!==null?l.cachePool:null),l!==null?zd(t,l):no(),Ud(t);else return a=t.lanes=536870912,D_(e,t,l!==null?l.baseLanes|n:n,n,a)}else l!==null?(Wl(t,l.cachePool),zd(t,l),Pn(),t.memoizedState=null):(e!==null&&Wl(t,null),no(),Pn());return Je(e,t,i,n),t.child}function Wi(e,t){return e!==null&&e.tag===22||t.stateNode!==null||(t.stateNode={_visibility:1,_pendingMarkers:null,_retryCache:null,_transitions:null}),t.sibling}function D_(e,t,n,a,i){var l=Zs();return l=l===null?null:{parent:Pe._currentValue,pool:l},t.memoizedState={baseLanes:n,cachePool:l},e!==null&&Wl(t,null),no(),Ud(t),e!==null&&va(e,t,a,!0),t.childLanes=i,null}function vr(e,t){return t=yr({mode:t.mode,children:t.children},e.mode),t.ref=e.ref,e.child=t,t.return=e,t}function M_(e,t,n){return wa(t,e.child,null,n),e=vr(t,t.pendingProps),e.flags|=2,Dt(t),t.memoizedState=null,e}function Iv(e,t,n){var a=t.pendingProps,i=(t.flags&128)!==0;if(t.flags&=-129,e===null){if(ue){if(a.mode==="hidden")return e=vr(t,a),t.lanes=536870912,e.memoizedState={baseLanes:0,cachePool:null},Wi(null,e);if(io(t),(e=Ue)?(e=of(e,Qt),e=e!==null&&e.data==="&"?e:null,e!==null&&(t.memoizedState={dehydrated:e,treeContext:zn!==null?{id:ln,overflow:rn}:null,retryLane:536870912,hydrationErrors:null},n=hd(e),n.return=t,t.child=n,tt=t,Ue=null)):e=null,e===null)throw Bn(t);return t.lanes=536870912,null}return vr(t,a)}var l=e.memoizedState;if(l!==null){var s=l.dehydrated;if(io(t),i)if(t.flags&256)t.flags&=-257,t=M_(e,t,n);else if(t.memoizedState!==null)t.child=e.child,t.flags|=128,t=null;else throw Error(d(558));else if(Xe||va(e,t,n,!1),i=(n&e.childLanes)!==0,Xe||i){if(Yn.current===null){if(a=ze,a!==null&&(s=xu(a,n),s!==0&&s!==l.retryLane))throw l.retryLane=s,_a(e,s),Tt(a,e,s),Eo;zr()}t=M_(e,t,n)}else e=l.treeContext,Ue=Kt(s.nextSibling),tt=t,ue=!0,Un=null,Qt=!1,e!==null&&xd(t,e),t=vr(t,a),t.flags|=134221824;return t}return e=bn(e.child,{mode:a.mode,children:a.children}),e.ref=t.ref,t.child=e,e.return=t,e}function ei(e,t){var n=t.ref;if(n===null)e!==null&&e.ref!==null&&(t.flags|=4194816);else{if(typeof n!="function"&&typeof n!="object")throw Error(d(284));(e===null||e.ref!==n)&&(t.flags|=4194816)}}function ko(e,t,n,a,i){return ya(t),n=so(e,t,n,a,void 0,i),a=oo(),e!==null&&!Xe?(co(e,t,i),En(e,t,i)):(ue&&a&&Xl(t),t.flags|=1,Je(e,t,n,i),t.child)}function z_(e,t,n,a,i,l){return ya(t),t.updateQueue=null,n=Vd(t,a,n,i),Bd(e),a=oo(),e!==null&&!Xe?(co(e,t,l),En(e,t,l)):(ue&&a&&Xl(t),t.flags|=1,Je(e,t,n,l),t.child)}function U_(e,t,n,a,i){if(ya(t),t.stateNode===null){var l=Ya,s=n.contextType;typeof s=="object"&&s!==null&&(l=ot(s)),l=new n(a,l),t.memoizedState=l.state!==null&&l.state!==void 0?l.state:null,l.updater=So,t.stateNode=l,l._reactInternals=t,l=t.stateNode,l.props=a,l.state=t.memoizedState,l.refs={},$s(t),s=n.contextType,l.context=typeof s=="object"&&s!==null?ot(s):Ya,l.state=t.memoizedState,s=n.getDerivedStateFromProps,typeof s=="function"&&(No(t,n,s,a),l.state=t.memoizedState),typeof n.getDerivedStateFromProps=="function"||typeof l.getSnapshotBeforeUpdate=="function"||typeof l.UNSAFE_componentWillMount!="function"&&typeof l.componentWillMount!="function"||(s=l.state,typeof l.componentWillMount=="function"&&l.componentWillMount(),typeof l.UNSAFE_componentWillMount=="function"&&l.UNSAFE_componentWillMount(),s!==l.state&&So.enqueueReplaceState(l,l.state,null),Xi(t,a,l,i),Ki(),l.state=t.memoizedState),typeof l.componentDidMount=="function"&&(t.flags|=4194308),a=!0}else if(e===null){l=t.stateNode;var u=t.memoizedProps,_=Sa(n,u);l.props=_;var h=l.context,S=n.contextType;s=Ya,typeof S=="object"&&S!==null&&(s=ot(S));var T=n.getDerivedStateFromProps;S=typeof T=="function"||typeof l.getSnapshotBeforeUpdate=="function",u=t.pendingProps!==u,S||typeof l.UNSAFE_componentWillReceiveProps!="function"&&typeof l.componentWillReceiveProps!="function"||(u||h!==s)&&N_(t,l,a,s),Hn=!1;var v=t.memoizedState;l.state=v,Xi(t,a,l,i),Ki(),h=t.memoizedState,u||v!==h||Hn?(typeof T=="function"&&(No(t,n,T,a),h=t.memoizedState),(_=Hn||w_(t,n,_,a,v,h,s))?(S||typeof l.UNSAFE_componentWillMount!="function"&&typeof l.componentWillMount!="function"||(typeof l.componentWillMount=="function"&&l.componentWillMount(),typeof l.UNSAFE_componentWillMount=="function"&&l.UNSAFE_componentWillMount()),typeof l.componentDidMount=="function"&&(t.flags|=4194308)):(typeof l.componentDidMount=="function"&&(t.flags|=4194308),t.memoizedProps=a,t.memoizedState=h),l.props=a,l.state=h,l.context=s,a=_):(typeof l.componentDidMount=="function"&&(t.flags|=4194308),a=!1)}else{l=t.stateNode,Ws(e,t),s=t.memoizedProps,S=Sa(n,s),l.props=S,T=t.pendingProps,v=l.context,h=n.contextType,_=Ya,typeof h=="object"&&h!==null&&(_=ot(h)),u=n.getDerivedStateFromProps,(h=typeof u=="function"||typeof l.getSnapshotBeforeUpdate=="function")||typeof l.UNSAFE_componentWillReceiveProps!="function"&&typeof l.componentWillReceiveProps!="function"||(s!==T||v!==_)&&N_(t,l,a,_),Hn=!1,v=t.memoizedState,l.state=v,Xi(t,a,l,i),Ki();var w=t.memoizedState;s!==T||v!==w||Hn||e!==null&&e.dependencies!==null&&Jl(e.dependencies)?(typeof u=="function"&&(No(t,n,u,a),w=t.memoizedState),(S=Hn||w_(t,n,S,a,v,w,_)||e!==null&&e.dependencies!==null&&Jl(e.dependencies))?(h||typeof l.UNSAFE_componentWillUpdate!="function"&&typeof l.componentWillUpdate!="function"||(typeof l.componentWillUpdate=="function"&&l.componentWillUpdate(a,w,_),typeof l.UNSAFE_componentWillUpdate=="function"&&l.UNSAFE_componentWillUpdate(a,w,_)),typeof l.componentDidUpdate=="function"&&(t.flags|=4),typeof l.getSnapshotBeforeUpdate=="function"&&(t.flags|=1024)):(typeof l.componentDidUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=4),typeof l.getSnapshotBeforeUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=1024),t.memoizedProps=a,t.memoizedState=w),l.props=a,l.state=w,l.context=_,a=S):(typeof l.componentDidUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=4),typeof l.getSnapshotBeforeUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=1024),a=!1)}return l=a,ei(e,t),a=(t.flags&128)!==0,l||a?(l=t.stateNode,n=a&&typeof n.getDerivedStateFromError!="function"?null:l.render(),t.flags|=1,e!==null&&a?(t.child=wa(t,e.child,null,i),t.child=wa(t,null,n,i)):Je(e,t,n,i),t.memoizedState=l.state,e=t.child):e=En(e,t,i),e}function B_(e,t,n,a){return fa(),t.flags|=256,Je(e,t,n,a),t.child}var To={dehydrated:null,treeContext:null,retryLane:0,hydrationErrors:null};function Co(e){return{baseLanes:e,cachePool:kd()}}function Ao(e,t,n){return e=e!==null?e.childLanes&~n:0,t&&(e|=Ut),e}function V_(e,t,n){var a=t.pendingProps,i=!1,l=(t.flags&128)!==0,s;if((s=l)||(s=e!==null&&e.memoizedState===null?!1:(ut.current&2)!==0),s&&(i=!0,t.flags&=-129),s=(t.flags&32)!==0,t.flags&=-33,e===null){if(ue){if(i?Qn(t):Pn(),(e=Ue)?(e=of(e,Qt),e=e!==null&&e.data!=="&"?e:null,e!==null&&(t.memoizedState={dehydrated:e,treeContext:zn!==null?{id:ln,overflow:rn}:null,retryLane:536870912,hydrationErrors:null},n=hd(e),n.return=t,t.child=n,tt=t,Ue=null)):e=null,e===null)throw Bn(t);return Cc(e)?t.lanes=32:t.lanes=536870912,null}return l=a.children,a=a.fallback,i?(Pn(),i=t.mode,l=yr({mode:"hidden",children:l},i),a=ma(a,i,n,null),l.return=t,a.return=t,l.sibling=a,t.child=l,a=t.child,a.memoizedState=Co(n),a.childLanes=Ao(e,s,n),t.memoizedState=To,Wi(null,a)):(Qn(t),Oo(t,l))}var u=e.memoizedState;if(u!==null){var _=u.dehydrated;if(_!==null)return Yv(e,t,l,s,a,_,u,n)}return i?(Pn(),i=a.fallback,l=t.mode,u=e.child,_=u.sibling,a=bn(u,{mode:"hidden",children:a.children}),a.subtreeFlags=u.subtreeFlags&1206910976,_!==null?i=bn(_,i):(i=ma(i,l,n,null),i.flags|=2),i.return=t,a.return=t,a.sibling=i,t.child=a,Wi(null,a),a=t.child,i=e.child.memoizedState,i===null?i=Co(n):(l=i.cachePool,l!==null?(u=Pe._currentValue,l=l.parent!==u?{parent:u,pool:u}:l):l=kd(),i={baseLanes:i.baseLanes|n,cachePool:l}),a.memoizedState=i,a.childLanes=Ao(e,s,n),t.memoizedState=To,Wi(e.child,a)):(Qn(t),n=e.child,e=n.sibling,n=bn(n,{mode:"visible",children:a.children}),n.return=t,n.sibling=null,e!==null&&(s=t.deletions,s===null?(t.deletions=[e],t.flags|=16):s.push(e)),t.child=n,t.memoizedState=null,n)}function Oo(e,t){return t=yr({mode:"visible",children:t},e.mode),t.return=e,e.child=t}function yr(e,t){return e=St(22,e,null,t),e.lanes=0,e}function hr(e,t,n){return wa(t,e.child,null,n),e=Oo(t,t.pendingProps.children),e.flags|=2,t.memoizedState=null,e}function Yv(e,t,n,a,i,l,s,u){if(n)return t.flags&256?(Qn(t),t.flags&=-257,hr(e,t,u)):t.memoizedState!==null?(Pn(),t.child=e.child,t.flags|=128,null):(Pn(),l=i.fallback,s=t.mode,i=yr({mode:"visible",children:i.children},s),l=ma(l,s,u,null),l.flags|=2,i.return=t,l.return=t,i.sibling=l,t.child=i,wa(t,e.child,null,u),i=t.child,i.memoizedState=Co(u),i.childLanes=Ao(e,a,u),t.memoizedState=To,Wi(null,i));if(Qn(t),Cc(l)){if(a=l.nextSibling&&l.nextSibling.dataset,a)var _=a.dgst;return a=_,a!==""&&(i=Error(d(419)),i.stack="",i.digest=a,Hi({value:i,source:null,stack:null})),hr(e,t,u)}if(Xe||va(e,t,u,!1),a=(u&e.childLanes)!==0,Xe||a){if(Yn.current!==null)return hr(e,t,u);if(a=ze,a!==null&&(i=xu(a,u),i!==0&&i!==s.retryLane))throw s.retryLane=i,_a(e,i),Tt(a,e,i),Eo;return Tc(l)||zr(),hr(e,t,u)}return Tc(l)?(t.flags|=192,t.child=e.child,null):(e=s.treeContext,Ue=Kt(l.nextSibling),tt=t,ue=!0,Un=null,Qt=!1,e!==null&&xd(t,e),t=Oo(t,i.children),t.flags|=134221824,t)}function L_(e,t,n){e.lanes|=t;var a=e.alternate;a!==null&&(a.lanes|=t),Zl(e.return,t,n)}function H_(e){for(var t=null;e!==null;){var n=e.alternate;n!==null&&lr(n)===null&&(t=e),e=e.sibling}return t}function gr(e,t,n,a,i,l){var s=e.memoizedState;s===null?e.memoizedState={isBackwards:t,rendering:null,renderingStartTime:0,last:a,tail:n,tailMode:i,treeForkCount:l}:(s.isBackwards=t,s.rendering=null,s.renderingStartTime=0,s.last=a,s.tail=n,s.tailMode=i,s.treeForkCount=l)}function qo(e){var t=e.child;for(e.child=null;t!==null;){var n=t.sibling;t.sibling=e.child,e.child=t,t=n}}function Ro(e,t,n){var a=t.pendingProps,i=a.revealOrder,l=a.tail;a=a.children;var s=ut.current;if(t.flags&128)return Fi(t,s),null;var u=(s&2)!==0;if(u?(s=s&1|2,t.flags|=128):s&=1,Fi(t,s),i==="backwards"&&e!==null?(qo(e),Je(e,t,a,n),qo(e)):Je(e,t,a,n),a=ue?Li:0,!u&&e!==null&&(e.flags&128)!==0)e:for(e=t.child;e!==null;){if(e.tag===13)e.memoizedState!==null&&L_(e,n,t);else if(e.tag===19)L_(e,n,t);else if(e.child!==null){e.child.return=e,e=e.child;continue}if(e===t)break e;for(;e.sibling===null;){if(e.return===null||e.return===t)break e;e=e.return}e.sibling.return=e.return,e=e.sibling}switch(i){case"backwards":n=H_(t.child),n===null?(i=t.child,t.child=null):(i=n.sibling,n.sibling=null,qo(t)),gr(t,!0,i,null,l,a);break;case"unstable_legacy-backwards":for(n=null,i=t.child,t.child=null;i!==null;){if(e=i.alternate,e!==null&&lr(e)===null){t.child=i;break}e=i.sibling,i.sibling=n,n=i,i=e}gr(t,!0,n,null,l,a);break;case"together":gr(t,!1,null,null,void 0,a);break;case"independent":t.memoizedState=null;break;default:n=H_(t.child),n===null?(i=t.child,t.child=null):(i=n.sibling,n.sibling=null),gr(t,!1,i,n,l,a)}return t.child}function G_(e,t,n){var a=t.pendingProps;return Vn(t,t.type,a.value),Je(e,t,a.children,n),t.child}function En(e,t,n){if(e!==null&&(t.dependencies=e.dependencies),Zn|=t.lanes,(n&t.childLanes)===0)if(e!==null){if(va(e,t,n,!1),(n&t.childLanes)===0)return null}else return null;if(e!==null&&t.child!==e.child)throw Error(d(153));if(t.child!==null){for(e=t.child,n=bn(e,e.pendingProps),t.child=n,n.return=t;e.sibling!==null;)e=e.sibling,n=n.sibling=bn(e,e.pendingProps),n.return=t;n.sibling=null}return t.child}function Do(e,t){return(e.lanes&t)!==0?!0:(e=e.dependencies,!!(e!==null&&Jl(e)))}function Qv(e,t,n){switch(t.tag){case 3:nn(t,t.stateNode.containerInfo),Vn(t,Pe,e.memoizedState.cache),fa();break;case 27:case 5:rs(t);break;case 4:nn(t,t.stateNode.containerInfo);break;case 10:Vn(t,t.type,t.memoizedProps.value);break;case 31:if(t.memoizedState!==null)return t.flags|=128,io(t),null;break;case 13:var a=t.memoizedState;if(a!==null){if(a.dehydrated!==null)return Qn(t),t.flags|=128,null;a=va(e,t,n,!1);var i=t.child.childLanes;return a||(n&i)!==0?V_(e,t,n):(Qn(t),e=En(e,t,n),e!==null?e.sibling:null)}Qn(t);break;case 19:if(t.flags&128)return Ro(e,t,n);if(i=(e.flags&128)!==0,a=(n&t.childLanes)!==0,a||(va(e,t,n,!1),a=(n&t.childLanes)!==0),i){if(a)return Ro(e,t,n);t.flags|=128}if(i=t.memoizedState,i!==null&&(i.rendering=null,i.tail=null,i.lastEffect=null),Fi(t,ut.current),a)break;return null;case 22:return t.lanes=0,R_(e,t,n,t.pendingProps);case 24:Vn(t,Pe,e.memoizedState.cache)}return En(e,t,n)}function I_(e,t,n){if(e!==null)if(e.memoizedProps!==t.pendingProps)Xe=!0;else{if(!Do(e,n)&&(t.flags&128)===0)return Xe=!1,Qv(e,t,n);Xe=(e.flags&131072)!==0}else Xe=!1,ue&&(t.flags&1048576)!==0&&bd(t,Li,t.index);switch(t.lanes=0,t.tag){case 16:e:{var a=t.pendingProps;if(e=ba(t.elementType),t.type=e,typeof e=="function")Ls(e)?(a=Sa(e,a),t.tag=1,t=U_(null,t,e,a,n)):(t.tag=0,t=ko(null,t,e,a,n));else{if(e!=null){var i=e.$$typeof;if(i===H){t.tag=11,t=A_(null,t,e,a,n);break e}else if(i===Ee){t.tag=14,t=O_(null,t,e,a,n);break e}else if(i===Le){t.tag=10,t.type=e,t=G_(null,t,n);break e}}throw t=ve(e)||e,Error(d(306,t,""))}}return t;case 0:return ko(e,t,t.type,t.pendingProps,n);case 1:return a=t.type,i=Sa(a,t.pendingProps),U_(e,t,a,i,n);case 3:e:{if(nn(t,t.stateNode.containerInfo),e===null)throw Error(d(387));a=t.pendingProps;var l=t.memoizedState;i=l.element,Ws(e,t),Xi(t,a,null,n);var s=t.memoizedState;if(a=s.cache,Vn(t,Pe,a),a!==l.cache&&Ks(t,[Pe],n,!0),Ki(),a=s.element,l.isDehydrated)if(l={element:a,isDehydrated:!1,cache:s.cache},t.updateQueue.baseState=l,t.memoizedState=l,t.flags&256){t=B_(e,t,a,n);break e}else if(a!==i){i=Gt(Error(d(424)),t),Hi(i),t=B_(e,t,a,n);break e}else{switch(e=t.stateNode.containerInfo,e.nodeType){case 9:e=e.body;break;default:e=e.nodeName==="HTML"?e.ownerDocument.body:e}for(Ue=Kt(e.firstChild),tt=t,ue=!0,Un=null,Qt=!0,n=Rd(t,null,a,n),t.child=n;n;)n.flags=n.flags&-3|134221824,n=n.sibling}else{if(fa(),a===i){t=En(e,t,n);break e}Je(e,t,a,n)}t=t.child}return t;case 26:return ei(e,t),e===null?(n=pf(t.type,null,t.pendingProps,null))?t.memoizedState=n:ue||(t.stateNode=Km(t.type,t.pendingProps,rt.current,t)):t.memoizedState=pf(t.type,e.memoizedProps,t.pendingProps,e.memoizedState),null;case 27:return rs(t),e===null&&ue&&(a=t.stateNode=df(t.type,t.pendingProps,rt.current),tt=t,Qt=!0,i=Ue,ea(t.type)?(Ac=i,Ue=Kt(a.firstChild)):Ue=i),Je(e,t,t.pendingProps.children,n),ei(e,t),e===null&&(t.flags|=4194304),t.child;case 5:return e===null&&ue&&((i=a=Ue)&&(a=Vy(a,t.type,t.pendingProps,Qt),a!==null?(t.stateNode=a,tt=t,Ue=Kt(a.firstChild),Qt=!1,i=!0):i=!1),i||Bn(t)),rs(t),i=t.type,l=t.pendingProps,s=e!==null?e.memoizedProps:null,a=l.children,xc(i,l)?a=null:s!==null&&xc(i,s)&&(t.flags|=32),t.memoizedState!==null&&(i=so(e,t,Dv,null,null,n),hi._currentValue=i),ei(e,t),Je(e,t,a,n),t.child;case 6:return e===null&&ue&&((e=n=Ue)&&(n=Ly(n,t.pendingProps,Qt),n!==null?(t.stateNode=n,tt=t,Ue=null,e=!0):e=!1),e||Bn(t)),null;case 13:return V_(e,t,n);case 4:return nn(t,t.stateNode.containerInfo),a=t.pendingProps,e===null?t.child=wa(t,null,a,n):Je(e,t,a,n),t.child;case 11:return A_(e,t,t.type,t.pendingProps,n);case 7:return a=t.pendingProps,ei(e,t),Je(e,t,a,n),t.child;case 8:return Je(e,t,t.pendingProps.children,n),t.child;case 12:return Je(e,t,t.pendingProps.children,n),t.child;case 10:return G_(e,t,n);case 9:return i=t.type._context,a=t.pendingProps.children,ya(t),i=ot(i),a=a(i),t.flags|=1,Je(e,t,a,n),t.child;case 14:return O_(e,t,t.type,t.pendingProps,n);case 15:return q_(e,t,t.type,t.pendingProps,n);case 19:return Ro(e,t,n);case 31:return Iv(e,t,n);case 22:return R_(e,t,n,t.pendingProps);case 24:return ya(t),a=ot(Pe),e===null?(i=Zs(),i===null&&(i=ze,l=Xs(),i.pooledCache=l,l.refCount++,l!==null&&(i.pooledCacheLanes|=n),i=l),t.memoizedState={parent:a,cache:i},$s(t),Vn(t,Pe,i)):((e.lanes&n)!==0&&(Ws(e,t),Xi(t,null,null,n),Ki()),i=e.memoizedState,l=t.memoizedState,i.parent!==a?(i={parent:a,cache:a},t.memoizedState=i,t.lanes===0&&(t.memoizedState=t.updateQueue.baseState=i),Vn(t,Pe,a)):(a=l.cache,Vn(t,Pe,a),a!==i.cache&&Ks(t,[Pe],n,!0))),Je(e,t,t.pendingProps.children,n),t.child;case 30:return t.stateNode===null&&(t.stateNode={autoName:null,paired:null,clones:null,ref:null}),a=t.pendingProps,a.name!=null&&a.name!=="auto"?t.flags|=e===null?18882560:18874368:ue&&Xl(t),e!==null&&e.memoizedProps.name!==a.name?t.flags|=4194816:ei(e,t),Je(e,t,a.children,n),t.child;case 29:throw t.pendingProps}throw Error(d(156,t.tag))}function kn(e){e.flags|=4}function Mo(e,t,n,a,i){var l;if((l=(e.mode&32)!==0)&&(l=n===null?gf(t,a):gf(t,a)&&(a.src!==n.src||a.srcSet!==n.srcSet)),l){if(e.flags|=16777216,(i&335544128)===i)if(e.stateNode.complete)e.flags|=8192;else if(Sm())e.flags|=8192;else throw xa=tr,Js}else e.flags&=-16777217}function Y_(e,t){if(t.type!=="stylesheet"||(t.state.loading&4)!==0)e.flags&=-16777217;else if(e.flags|=16777216,!bf(t))if(Sm())e.flags|=8192;else throw xa=tr,Js}function br(e,t){t!==null&&(e.flags|=4),e.flags&16384&&(t=e.tag!==22?hu():536870912,e.lanes|=t,li|=t)}function el(e,t){if(!ue)switch(e.tailMode){case"visible":break;case"collapsed":for(var n=e.tail,a=null;n!==null;)n.alternate!==null&&(a=n),n=n.sibling;a===null?t||e.tail===null?e.tail=null:e.tail.sibling=null:a.sibling=null;break;default:for(t=e.tail,n=null;t!==null;)t.alternate!==null&&(n=t),t=t.sibling;n===null?e.tail=null:n.sibling=null}}function Be(e){var t=e.alternate!==null&&e.alternate.child===e.child,n=0,a=0;if(t)for(var i=e.child;i!==null;)n|=i.lanes|i.childLanes,a|=i.subtreeFlags&1206910976,a|=i.flags&1206910976,i.return=e,i=i.sibling;else for(i=e.child;i!==null;)n|=i.lanes|i.childLanes,a|=i.subtreeFlags,a|=i.flags,i.return=e,i=i.sibling;return e.subtreeFlags|=a,e.childLanes=n,t}function Pv(e,t,n){var a=t.pendingProps;switch(Is(t),t.tag){case 16:case 15:case 0:case 11:case 7:case 8:case 12:case 9:case 14:return Be(t),null;case 1:return Be(t),null;case 3:return n=t.stateNode,a=null,e!==null&&(a=e.memoizedState.cache),t.memoizedState.cache!==a&&(t.flags|=2048),Nn(Pe),Oa(),n.pendingContext&&(n.context=n.pendingContext,n.pendingContext=null),(e===null||e.child===null)&&(Ka(t)?kn(t):e===null||e.memoizedState.isDehydrated&&(t.flags&256)===0||(t.flags|=1024,Qs())),Be(t),null;case 26:var i=t.type,l=t.memoizedState;return e===null?(kn(t),l!==null?(Be(t),Y_(t,l)):(Be(t),Mo(t,i,null,a,n))):l?l!==e.memoizedState?(kn(t),Be(t),Y_(t,l)):(Be(t),t.flags&=-16777217):(e=e.memoizedProps,e!==a&&kn(t),Be(t),Mo(t,i,e,a,n)),null;case 27:if(jl(t),n=rt.current,i=t.type,e!==null&&t.stateNode!=null)e.memoizedProps!==a&&kn(t);else{if(!a){if(t.stateNode===null)throw Error(d(166));return Be(t),t.subtreeFlags&=-33554433,null}e=Ye.current,Ka(t)?wd(t):(e=df(i,a,n),t.stateNode=e,kn(t))}return Be(t),t.subtreeFlags&=-33554433,null;case 5:if(jl(t),i=t.type,e!==null&&t.stateNode!=null)e.memoizedProps!==a&&kn(t);else{if(!a){if(t.stateNode===null)throw Error(d(166));return Be(t),t.subtreeFlags&=-33554433,null}if(l=Ye.current,Ka(t))wd(t);else{var s=dl(rt.current);switch(l){case 1:l=s.createElementNS("http://www.w3.org/2000/svg",i);break;case 2:l=s.createElementNS("http://www.w3.org/1998/Math/MathML",i);break;default:switch(i){case"svg":l=s.createElementNS("http://www.w3.org/2000/svg",i);break;case"math":l=s.createElementNS("http://www.w3.org/1998/Math/MathML",i);break;case"script":l=s.createElement("div"),l.innerHTML="<script><\/script>",l=l.removeChild(l.firstChild);break;case"select":l=typeof a.is=="string"?s.createElement("select",{is:a.is}):s.createElement("select"),a.multiple?l.multiple=!0:a.size&&(l.size=a.size);break;default:l=typeof a.is=="string"?s.createElement(i,{is:a.is}):s.createElement(i)}}l[st]=t,l[Nt]=a;e:for(s=t.child;s!==null;){if(s.tag===5||s.tag===6)l.appendChild(s.stateNode);else if(s.tag!==4&&s.tag!==27&&s.child!==null){s.child.return=s,s=s.child;continue}if(s===t)break e;for(;s.sibling===null;){if(s.return===null||s.return===t)break e;s=s.return}s.sibling.return=s.return,s=s.sibling}t.stateNode=l;e:switch(_t(l,i,a),i){case"button":case"input":case"select":case"textarea":a=!!a.autoFocus;break e;case"img":a=!0;break e;default:a=!1}a&&kn(t)}}return Be(t),t.subtreeFlags&=-33554433,Mo(t,t.type,e===null?null:e.memoizedProps,t.pendingProps,n),null;case 6:if(e&&t.stateNode!=null)e.memoizedProps!==a&&kn(t);else{if(typeof a!="string"&&t.stateNode===null)throw Error(d(166));if(e=rt.current,Ka(t)){if(e=t.stateNode,n=t.memoizedProps,a=null,i=tt,i!==null)switch(i.tag){case 27:case 5:a=i.memoizedProps}e[st]=t,e=!!(e.nodeValue===n||a!==null&&a.suppressHydrationWarning===!0||Im(e.nodeValue,n)),e||Bn(t,!0)}else e=dl(e).createTextNode(a),e[st]=t,t.stateNode=e}return Be(t),null;case 31:if(n=t.memoizedState,e===null||e.memoizedState!==null){if(a=Ka(t),n!==null){if(e===null){if(!a)throw Error(d(318));if(e=t.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(557));e[st]=t}else fa(),(t.flags&128)===0&&(t.memoizedState=null),t.flags|=4;Be(t),e=!1}else n=Qs(),e!==null&&e.memoizedState!==null&&(e.memoizedState.hydrationErrors=n),e=!0;if(!e)return t.flags&256?(Dt(t),t):(Dt(t),null);if((t.flags&128)!==0)throw Error(d(558))}return Be(t),null;case 13:if(a=t.memoizedState,e===null||e.memoizedState!==null&&e.memoizedState.dehydrated!==null){if(i=Ka(t),a!==null&&a.dehydrated!==null){if(e===null){if(!i)throw Error(d(318));if(i=t.memoizedState,i=i!==null?i.dehydrated:null,!i)throw Error(d(317));i[st]=t}else fa(),(t.flags&128)===0&&(t.memoizedState=null),t.flags|=4;Be(t),i=!1}else i=Qs(),e!==null&&e.memoizedState!==null&&(e.memoizedState.hydrationErrors=i),i=!0;if(!i)return t.flags&256?(Dt(t),t):(Dt(t),null)}return Dt(t),(t.flags&128)!==0?(t.lanes=n,t):(n=a!==null,e=e!==null&&e.memoizedState!==null,n&&(a=t.child,i=null,a.alternate!==null&&a.alternate.memoizedState!==null&&a.alternate.memoizedState.cachePool!==null&&(i=a.alternate.memoizedState.cachePool.pool),l=null,a.memoizedState!==null&&a.memoizedState.cachePool!==null&&(l=a.memoizedState.cachePool.pool),l!==i&&(a.flags|=2048)),n!==e&&n&&(t.child.flags|=8192),br(t,t.updateQueue),Be(t),null);case 4:return Oa(),e===null&&vc(t.stateNode.containerInfo),t.flags|=67108864,Be(t),null;case 10:return Nn(t.type),Be(t),null;case 19:if(lo(t),a=t.memoizedState,a===null)return Be(t),null;if(i=(t.flags&128)!==0,l=a.rendering,l===null)if(i)el(a,!1);else{if(Ie!==0||e!==null&&(e.flags&128)!==0)for(e=t.child;e!==null;){if(l=lr(e),l!==null){for(t.flags|=128,el(a,!1),e=l.updateQueue,t.updateQueue=e,br(t,e),t.subtreeFlags=0,e=n,n=t.child;n!==null;)yd(n,e),n=n.sibling;return Fi(t,ut.current&1|2),ue&&xn(t,a.treeForkCount),t.child}e=e.sibling}a.tail!==null&&Ct()>qr&&(t.flags|=128,i=!0,el(a,!1),t.lanes=4194304)}else{if(!i)if(e=lr(l),e!==null){if(t.flags|=128,i=!0,e=e.updateQueue,t.updateQueue=e,br(t,e),el(a,!0),a.tail===null&&a.tailMode!=="collapsed"&&a.tailMode!=="visible"&&!l.alternate&&!ue)return Be(t),null}else 2*Ct()-a.renderingStartTime>qr&&n!==536870912&&(t.flags|=128,i=!0,el(a,!1),t.lanes=4194304);a.isBackwards?(l.sibling=t.child,t.child=l):(e=a.last,e!==null?e.sibling=l:t.child=l,a.last=l)}if(a.tail!==null){e=a.tail;e:{for(n=e;n!==null;){if(n.alternate!==null){n=!1;break e}n=n.sibling}n=!0}return a.rendering=e,a.tail=e.sibling,a.renderingStartTime=Ct(),e.sibling=null,l=ut.current,l=i?l&1|2:l&1,a.tailMode==="visible"||a.tailMode==="collapsed"||!n||ue?Fi(t,l):(n=l,V(ct,t),V(ut,n),ft===null&&(ft=t)),ue&&xn(t,a.treeForkCount),e}return Be(t),null;case 22:case 23:return Dt(t),ao(),a=t.memoizedState!==null,e!==null?e.memoizedState!==null!==a&&(t.flags|=8192):a&&(t.flags|=8192),a?(n&536870912)!==0&&(t.flags&128)===0&&(Be(t),t.subtreeFlags&6&&(t.flags|=8192)):Be(t),n=t.updateQueue,n!==null&&br(t,n.retryQueue),n=null,e!==null&&e.memoizedState!==null&&e.memoizedState.cachePool!==null&&(n=e.memoizedState.cachePool.pool),a=null,t.memoizedState!==null&&t.memoizedState.cachePool!==null&&(a=t.memoizedState.cachePool.pool),a!==n&&(t.flags|=2048),e!==null&&Te(ga),null;case 24:return n=null,e!==null&&(n=e.memoizedState.cache),t.memoizedState.cache!==n&&(t.flags|=2048),Nn(Pe),Be(t),null;case 25:return null;case 30:return t.flags|=33554432,Be(t),null}throw Error(d(156,t.tag))}function Kv(e,t){switch(Is(t),t.tag){case 1:return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 3:return Nn(Pe),Oa(),e=t.flags,(e&65536)!==0&&(e&128)===0?(t.flags=e&-65537|128,t):null;case 26:case 27:case 5:return jl(t),null;case 31:if(t.memoizedState!==null){if(Dt(t),t.alternate===null)throw Error(d(340));fa()}return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 13:if(Dt(t),e=t.memoizedState,e!==null&&e.dehydrated!==null){if(t.alternate===null)throw Error(d(340));fa()}return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 19:return lo(t),e=t.flags,e&65536?(t.flags=e&-65537|128,e=t.memoizedState,e!==null&&(e.rendering=null,e.tail=null),t.flags|=4,t):null;case 4:return Oa(),null;case 10:return Nn(t.type),null;case 22:case 23:return Dt(t),ao(),e!==null&&Te(ga),e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 24:return Nn(Pe),null;case 25:return null;default:return null}}function Q_(e,t){switch(Is(t),t.tag){case 3:Nn(Pe),Oa();break;case 26:case 27:case 5:jl(t);break;case 4:Oa();break;case 31:t.memoizedState!==null&&Dt(t);break;case 13:Dt(t);break;case 19:lo(t);break;case 10:Nn(t.type);break;case 22:case 23:Dt(t),ao(),e!==null&&Te(ga);break;case 24:Nn(Pe)}}function tl(e,t){try{var n=t.updateQueue,a=n!==null?n.lastEffect:null;if(a!==null){var i=a.next;n=i;do{if((n.tag&e)===e){a=void 0;var l=n.create,s=n.inst;a=l(),s.destroy=a}n=n.next}while(n!==i)}}catch(u){Oe(t,t.return,u)}}function Kn(e,t,n){try{var a=t.updateQueue,i=a!==null?a.lastEffect:null;if(i!==null){var l=i.next;a=l;do{if((a.tag&e)===e){var s=a.inst,u=s.destroy;if(u!==void 0){s.destroy=void 0,i=t;var _=n,h=u;try{h()}catch(S){Oe(i,_,S)}}}a=a.next}while(a!==l)}}catch(S){Oe(t,t.return,S)}}function P_(e){var t=e.updateQueue;if(t!==null){var n=e.stateNode;try{Md(t,n)}catch(a){Oe(e,e.return,a)}}}function K_(e,t,n){n.props=Sa(e.type,e.memoizedProps),n.state=e.memoizedState;try{n.componentWillUnmount()}catch(a){Oe(e,t,a)}}function sn(e,t){try{var n=e.ref;if(n!==null){switch(e.tag){case 26:case 27:case 5:var a=e.stateNode;break;case 30:var i=e.stateNode,l=hn(e.memoizedProps,i);(i.ref===null||i.ref.name!==l)&&(i.ref=ef(l)),a=i.ref;break;case 7:if(e.stateNode===null){var s=new Vt(e);g(e.child,!1,Uy,s,void 0,void 0),e.stateNode=s}a=e.stateNode;break;default:a=e.stateNode}typeof n=="function"?e.refCleanup=n(a):n.current=a}}catch(u){Oe(e,t,u)}}function dt(e,t){var n=e.ref,a=e.refCleanup;if(n!==null)if(typeof a=="function")try{a()}catch(i){Oe(e,t,i)}finally{e.refCleanup=null,e=e.alternate,e!=null&&(e.refCleanup=null)}else if(typeof n=="function")try{n(null)}catch(i){Oe(e,t,i)}else n.current=null}function xr(e,t){if((e.tag===5||e.tag===27||e.tag===6)&&e.alternate===null&&t!==null)for(var n=0;n<t.length;n++)sf(e.stateNode,t[n])}function X_(e){for(var t=e.return;t!==null&&(Uo(t)&&sf(e.stateNode,t.stateNode),!zo(t));)t=t.return}function nl(e){for(var t=e.return;t!==null&&(Uo(t)&&By(e.stateNode,t.stateNode),!zo(t));)t=t.return}function zo(e){return e.tag===5||e.tag===3||e.tag===27}function Uo(e){return e&&e.tag===7&&e.stateNode!==null}function Bo(e){var t=e.type,n=e.memoizedProps,a=e.stateNode;try{e:switch(t){case"button":case"input":case"select":case"textarea":n.autoFocus&&a.focus();break e;case"img":n.src?a.src=n.src:n.srcSet&&(a.srcset=n.srcSet)}}catch(i){Oe(e,e.return,i)}}function Vo(e,t,n){try{var a=e.stateNode;gy(a,e.type,n,t),a[Nt]=t}catch(i){Oe(e,e.return,i)}}function F_(e){return e.tag===5||e.tag===3||e.tag===26||e.tag===27&&ea(e.type)||e.tag===4}function Lo(e){e:for(;;){for(;e.sibling===null;){if(e.return===null||F_(e.return))return null;e=e.return}for(e.sibling.return=e.return,e=e.sibling;e.tag!==5&&e.tag!==6&&e.tag!==18;){if(e.tag===27&&ea(e.type)||e.flags&2||e.child===null||e.tag===4)continue e;e.child.return=e,e=e.child}if(!(e.flags&2))return e.stateNode}}function Ho(e,t,n,a){var i=e.tag;if(i===5||i===6)i=e.stateNode,t?(n.nodeType===9?n.body:n.nodeName==="HTML"?n.ownerDocument.body:n).insertBefore(i,t):(t=n.nodeType===9?n.body:n.nodeName==="HTML"?n.ownerDocument.body:n,t.appendChild(i),n=n._reactRootContainer,n!=null||t.onclick!==null||(t.onclick=an)),xr(e,a),Se=!0;else if(i!==4&&(i===27&&(xr(e,a),a=null,ea(e.type)&&(n=e.stateNode,t=null)),e=e.child,e!==null))for(Ho(e,t,n,a),e=e.sibling;e!==null;)Ho(e,t,n,a),e=e.sibling}function wr(e,t,n,a){var i=e.tag;if(i===5||i===6)i=e.stateNode,t?n.insertBefore(i,t):n.appendChild(i),xr(e,a),Se=!0;else if(i!==4&&(i===27&&(xr(e,a),a=null,ea(e.type)&&(n=e.stateNode)),e=e.child,e!==null))for(wr(e,t,n,a),e=e.sibling;e!==null;)wr(e,t,n,a),e=e.sibling}function Z_(e){var t=e.stateNode,n=e.memoizedProps;try{for(var a=e.type,i=t.attributes;i.length;)t.removeAttributeNode(i[0]);_t(t,a,n),t[st]=e,t[Nt]=n}catch(l){Oe(e,e.return,l)}}var Nr=!1,Mt=null;function J_(e){(e.tag===30||(e.subtreeFlags&33554432)!==0)&&(Nr=!0)}var on=null;function $_(){var e=on;return on=null,e}var jt=0;function ti(e,t,n,a,i){return jt=0,W_(e.child,t,n,a,i)}function W_(e,t,n,a,i){for(var l=!1;e!==null;){if(e.tag===5){var s=e.stateNode;if(a!==null){var u=Sc(s);a.push(u),u.view&&(l=!0)}else l||Sc(s).view&&(l=!0);Nr=!0,$m(s,jt===0?t:t+"_"+jt,n),jt++}else(e.tag!==22||e.memoizedState===null)&&(e.tag===30&&i||W_(e.child,t,n,a,i)&&(l=!0));e=e.sibling}return l}function cn(e,t){for(;e!==null;)e.tag===5?Wm(e.stateNode,e.memoizedProps):(e.tag!==22||e.memoizedState===null)&&(e.tag===30&&t||cn(e.child,t)),e=e.sibling}function Sr(e){if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if((e.tag!==22||e.memoizedState===null)&&(Sr(e),e.tag===30&&(e.flags&18874368)!==0&&e.stateNode.paired)){var t=e.memoizedProps;if(t.name==null||t.name==="auto")throw Error(d(544));var n=t.name;t=gn(t.default,t.share),t!=="none"&&(ti(e,n,t,null,!1)||cn(e.child,!1))}e=e.sibling}}function Go(e,t){if(e.tag===30){var n=e.stateNode,a=e.memoizedProps,i=hn(a,n),l=gn(a.default,n.paired?a.share:a.enter);l!=="none"?ti(e,i,l,null,!1)?(Sr(e),n.paired||t||ci(e,a.onEnter)):cn(e.child,!1):Sr(e)}else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)Go(e,t),e=e.sibling;else Sr(e)}function Io(e){if(Mt!==null&&Mt.size!==0){var t=Mt;if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if(e.tag!==22||e.memoizedState===null){if(e.tag===30&&(e.flags&18874368)!==0){var n=e.memoizedProps,a=n.name;if(a!=null&&a!=="auto"){var i=t.get(a);if(i!==void 0){var l=gn(n.default,n.share);if(l!=="none"&&(ti(e,a,l,null,!1)?(l=e.stateNode,i.paired=l,l.paired=i,ci(e,n.onShare)):cn(e.child,!1)),t.delete(a),t.size===0)break}}}Io(e)}e=e.sibling}}}function Yo(e){if(e.tag===30){var t=e.memoizedProps,n=hn(t,e.stateNode),a=Mt!==null?Mt.get(n):void 0,i=gn(t.default,a!==void 0?t.share:t.exit);i!=="none"&&(ti(e,n,i,null,!1)?a!==void 0?(i=e.stateNode,a.paired=i,i.paired=a,Mt.delete(n),ci(e,t.onShare)):ci(e,t.onExit):cn(e.child,!1)),Mt!==null&&Io(e)}else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)Yo(e),e=e.sibling;else Mt!==null&&Io(e)}function em(e){for(e=e.child;e!==null;){if(e.tag===30){var t=e.memoizedProps,n=hn(t,e.stateNode);t=gn(t.default,t.update),e.flags&=-5,t!=="none"&&ti(e,n,t,e.memoizedState=[],!1)}else(e.subtreeFlags&33554432)!==0&&em(e);e=e.sibling}}function Qo(e){if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if(e.tag!==22||e.memoizedState===null){if(e.tag===30&&(e.flags&18874368)!==0){var t=e.stateNode;t.paired!==null&&(t.paired=null,cn(e.child,!1))}Qo(e)}e=e.sibling}}function jr(e){if(e.tag===30)e.stateNode.paired=null,cn(e.child,!1),Qo(e);else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)jr(e),e=e.sibling;else Qo(e)}function tm(e){for(e=e.child;e!==null;)e.tag===30?cn(e.child,!1):(e.subtreeFlags&33554432)!==0&&tm(e),e=e.sibling}function Po(e,t,n,a,i,l,s){for(var u=!1;t!==null;){if(t.tag===5){var _=t.stateNode;if(l!==null&&jt<l.length){var h=l[jt],S=Sc(_);(h.view||S.view)&&(u=!0);var T;if(T=(e.flags&4)===0)if(S.clip)T=!0;else{T=h.rect;var v=S.rect;T=T.y!==v.y||T.x!==v.x||T.height!==v.height||T.width!==v.width}T&&(e.flags|=4),S.abs?S=!h.abs:(h=h.rect,S=S.rect,S=h.height!==S.height||h.width!==S.width),S&&(e.flags|=32)}else e.flags|=32;(e.flags&4)!==0&&$m(_,jt===0?n:n+"_"+jt,i),u&&(e.flags&4)!==0||(on===null&&(on=[]),on.push(_,jt===0?a:a+"_"+jt,t.memoizedProps)),jt++}else(t.tag!==22||t.memoizedState===null)&&(t.tag===30&&s?e.flags|=t.flags&32:Po(e,t.child,n,a,i,l,s)&&(u=!0));t=t.sibling}return u}function nm(e,t){for(e=e.child;e!==null;){if(e.tag===30){var n=e.memoizedProps,a=e.stateNode,i=hn(n,a),l=gn(n.default,n.update),s;s=e.memoizedState,e.memoizedState=null,a=e;var u=e.child;jt=0,i=Po(a,u,i,i,l,s,!1),(e.flags&4)!==0&&i&&ci(e,n.onUpdate)}else(e.subtreeFlags&33554432)!==0&&nm(e);e=e.sibling}}var nt=!1,Ce=!1,un=!1,Ko=!1,am=typeof WeakSet=="function"?WeakSet:Set,at=null,dn=!1,al=!1,Er=!1,Xo=!1;function Xv(e,t,n){if(e=e.containerInfo,gc=gi,e=sd(e),Rs(e)){if("selectionStart"in e)var a={start:e.selectionStart,end:e.selectionEnd};else e:{a=(a=e.ownerDocument)&&a.defaultView||window;var i=a.getSelection&&a.getSelection();if(i&&i.rangeCount!==0){a=i.anchorNode;var l=i.anchorOffset,s=i.focusNode;i=i.focusOffset;try{a.nodeType,s.nodeType}catch{a=null;break e}var u=0,_=-1,h=-1,S=0,T=0,v=e,w=null;t:for(;;){for(var G;v!==a||l!==0&&v.nodeType!==3||(_=u+l),v!==s||i!==0&&v.nodeType!==3||(h=u+i),v.nodeType===3&&(u+=v.nodeValue.length),(G=v.firstChild)!==null;)w=v,v=G;for(;;){if(v===e)break t;if(w===a&&++S===l&&(_=u),w===s&&++T===i&&(h=u),(G=v.nextSibling)!==null)break;v=w,w=v.parentNode}v=G}a=_===-1||h===-1?null:{start:_,end:h}}else a=null}a=a||{start:0,end:0}}else a=null;for(bc={focusedElem:e,selectionRange:a},gi=!1,n=(n&335544064)===n,at=t,t=n?9270:1024;at!==null;){if(e=at,n&&(a=e.deletions,a!==null))for(l=0;l<a.length;l++)n&&Yo(a[l]);if(e.alternate===null&&(e.flags&2)!==0)n&&J_(e),kr(n);else{if(e.tag===22){if(a=e.alternate,e.memoizedState!==null){a!==null&&a.memoizedState===null&&n&&Yo(a),kr(n);continue}else if(a!==null&&a.memoizedState!==null){n&&J_(e),kr(n);continue}}a=e.child,(e.subtreeFlags&t)!==0&&a!==null?(a.return=e,at=a):(n&&em(e),kr(n))}}Mt=null}function kr(e){for(;at!==null;){var t=at,n=e,a=t.alternate,i=t.flags;switch(t.tag){case 0:case 11:case 15:break;case 1:if((i&1024)!==0&&a!==null){n=void 0,i=a.memoizedProps,a=a.memoizedState;var l=t.stateNode;try{var s=Sa(t.type,i);n=l.getSnapshotBeforeUpdate(s,a),l.__reactInternalSnapshotBeforeUpdate=n}catch(u){Oe(t,t.return,u)}}break;case 3:if((i&1024)!==0){if(a=t.stateNode.containerInfo,n=a.nodeType,n===9)kc(a);else if(n===1)switch(a.nodeName){case"HEAD":case"HTML":case"BODY":kc(a);break;default:a.textContent=""}}break;case 5:case 26:case 27:case 6:case 4:case 17:break;case 30:n&&a!==null&&(n=hn(a.memoizedProps,a.stateNode),i=t.memoizedProps,i=gn(i.default,i.update),i!=="none"&&ti(a,n,i,a.memoizedState=[],!0));break;default:if((i&1024)!==0)throw Error(d(163))}if(a=t.sibling,a!==null){a.return=t.return,at=a;break}at=t.return}}function im(e,t,n){var a=n.flags;switch(n.tag){case 0:case 11:case 15:_n(e,n),a&4&&tl(5,n);break;case 1:if(_n(e,n),a&4)if(e=n.stateNode,t===null)try{e.componentDidMount()}catch(s){Oe(n,n.return,s)}else{var i=Sa(n.type,t.memoizedProps);t=t.memoizedState;try{e.componentDidUpdate(i,t,e.__reactInternalSnapshotBeforeUpdate)}catch(s){Oe(n,n.return,s)}}a&64&&P_(n),a&512&&sn(n,n.return);break;case 3:if(_n(e,n),a&64&&(e=n.updateQueue,e!==null)){if(t=null,n.child!==null)switch(n.child.tag){case 27:case 5:t=n.child.stateNode;break;case 1:t=n.child.stateNode}try{Md(e,t)}catch(s){Oe(n,n.return,s)}}break;case 27:t===null&&a&4&&Z_(n);case 26:case 5:_n(e,n),t===null&&a&4&&Bo(n),a&512&&sn(n,n.return);break;case 12:_n(e,n);break;case 31:_n(e,n),a&4&&om(e,n);break;case 13:_n(e,n),a&4&&cm(e,n),a&64&&(e=n.memoizedState,e!==null&&(e=e.dehydrated,e!==null&&(n=ry.bind(null,n),Hy(e,n))));break;case 22:if(a=n.memoizedState!==null||nt,!a){var l=t!==null&&t.memoizedState!==null||Ce;t=nt,i=Ce,nt=a,(Ce=l)&&!i?(a=2,(n.subtreeFlags&8772)!==0&&(a|=1),Wt(e,n,a)):_n(e,n),nt=t,Ce=i}break;case 30:_n(e,n),a&512&&sn(n,n.return);break;case 7:a&512&&sn(n,n.return);default:_n(e,n)}}function Fo(e,t){for(e=e.child;e!==null;)lm(e,t),e=e.sibling}function lm(e,t){switch(e.tag){case 5:case 26:try{var n=e.stateNode;if(t){var a=n.style;typeof a.setProperty=="function"?a.setProperty("display","none","important"):a.display="none"}else{var i=e.stateNode,l=e.memoizedProps.style,s=l!=null&&l.hasOwnProperty("display")?l.display:null;i.style.display=s==null||typeof s=="boolean"?"":(""+s).trim()}}catch(_){Oe(e,e.return,_)}Zo(e,t);break;case 6:try{e.stateNode.nodeValue=t?"":e.memoizedProps,Se=!0}catch(_){Oe(e,e.return,_)}break;case 18:try{var u=e.stateNode;t?Jm(u,!0):Jm(e.stateNode,!1)}catch(_){Oe(e,e.return,_)}break;case 22:case 23:e.memoizedState===null&&Fo(e,t);break;default:Fo(e,t)}}function Zo(e,t){if(e.subtreeFlags&67108864)for(e=e.child;e!==null;){e:{var n=e,a=t;switch(n.tag){case 4:lm(n,a);break e;case 22:n.memoizedState===null&&Zo(n,a);break e;default:Zo(n,a)}}e=e.sibling}}function rm(e){var t=e.alternate;t!==null&&(e.alternate=null,rm(t)),e.child=null,e.deletions=null,e.sibling=null,e.tag===5&&(t=e.stateNode,t!==null&&ql(t)),e.stateNode=null,e.return=null,e.dependencies=null,e.memoizedProps=null,e.memoizedState=null,e.pendingProps=null,e.stateNode=null,e.updateQueue=null}var Ve=null,Et=!1;function Jt(e,t,n){for(n=n.child;n!==null;)sm(e,t,n),n=n.sibling}function sm(e,t,n){if(At&&typeof At.onCommitFiberUnmount=="function")try{At.onCommitFiberUnmount(Ei,n)}catch{}switch(n.tag){case 26:Ce||dt(n,t),Jt(e,t,n),n.memoizedState?n.memoizedState.count--:n.stateNode&&!Ce&&(n=n.stateNode,n.parentNode.removeChild(n));break;case 27:Ce||dt(n,t),nl(n);var a=Ve,i=Et;ea(n.type)&&(Ve=n.stateNode,Et=!1),Jt(e,t,n),_f(n.stateNode,n.type,n.memoizedProps),Ve=a,Et=i;break;case 5:Ce||dt(n,t),nl(n);case 6:if(n.tag===6&&nl(n),a=Ve,i=Et,Ve=null,Jt(e,t,n),Ve=a,Et=i,Ve!==null)if(Et)try{(Ve.nodeType===9?Ve.body:Ve.nodeName==="HTML"?Ve.ownerDocument.body:Ve).removeChild(n.stateNode),Se=!0}catch(l){Oe(n,t,l)}else try{Ve.removeChild(n.stateNode),Se=!0}catch(l){Oe(n,t,l)}break;case 18:Ve!==null&&(Et?(e=Ve,Zm(e.nodeType===9?e.body:e.nodeName==="HTML"?e.ownerDocument.body:e,n.stateNode),bi(e)):Zm(Ve,n.stateNode));break;case 4:a=Ve,i=Et,Ve=n.stateNode.containerInfo,Et=!0,Jt(e,t,n),Ve=a,Et=i;break;case 0:case 11:case 14:case 15:Kn(2,n,t),Ce||Kn(4,n,t),Jt(e,t,n);break;case 1:Ce||(dt(n,t),a=n.stateNode,typeof a.componentWillUnmount=="function"&&K_(n,t,a)),Jt(e,t,n);break;case 21:Jt(e,t,n);break;case 22:Ce=(a=Ce)||n.memoizedState!==null,Jt(e,t,n),Ce=a;break;case 30:dt(n,t),Jt(e,t,n);break;case 7:Ce||dt(n,t),Jt(e,t,n);break;default:Jt(e,t,n)}}function om(e,t){if(t.memoizedState===null&&(e=t.alternate,e!==null&&(e=e.memoizedState,e!==null))){e=e.dehydrated;try{bi(e)}catch(n){Oe(t,t.return,n)}}}function cm(e,t){if(t.memoizedState===null&&(e=t.alternate,e!==null&&(e=e.memoizedState,e!==null&&(e=e.dehydrated,e!==null))))try{bi(e)}catch(n){Oe(t,t.return,n)}}function Fv(e){switch(e.tag){case 31:case 13:case 19:var t=e.stateNode;return t===null&&(t=e.stateNode=new am),t;case 22:return e=e.stateNode,t=e._retryCache,t===null&&(t=e._retryCache=new am),t;default:throw Error(d(435,e.tag))}}function Tr(e,t){var n=Fv(e);t.forEach(function(a){if(!n.has(a)){n.add(a);var i=sy.bind(null,e,a);a.then(i,i)}})}function yt(e,t,n){var a=t.deletions;if(a!==null)for(var i=0;i<a.length;i++){var l=a[i],s=e,u=t,_=u;e:for(;_!==null;){switch(_.tag){case 27:if(ea(_.type)){Ve=_.stateNode,Et=!1;break e}break;case 5:Ve=_.stateNode,Et=!1;break e;case 3:case 4:Ve=_.stateNode.containerInfo,Et=!0;break e}_=_.return}if(Ve===null)throw Error(d(160));sm(s,u,l),Ve=null,Et=!1,s=l.alternate,s!==null&&(s.return=null),l.return=null}if(t.subtreeFlags&13886)for(t=t.child;t!==null;)um(t,e,n),t=t.sibling}var $t=null;function um(e,t,n){var a=e.alternate,i=e.flags;switch(e.tag){case 0:case 11:case 14:case 15:if(i&4&&(a=e.updateQueue,a=a!==null?a.events:null,a!==null))for(var l=0;l<a.length;l++){var s=a[l];s.ref.impl=s.nextImpl}yt(t,e,n),ht(e),i&4&&(Kn(3,e,e.return),tl(3,e),Kn(5,e,e.return));break;case 1:yt(t,e,n),ht(e),i&512&&(Ce||a===null||dt(a,a.return)),i&64&&nt&&(e=e.updateQueue,e!==null&&(t=e.callbacks,t!==null&&(n=e.shared.hiddenCallbacks,e.shared.hiddenCallbacks=n===null?t:n.concat(t))));break;case 26:if(l=$t,yt(t,e,n),ht(e),i&512&&(Ce||a===null||dt(a,a.return)),i&4)if(i=a!==null?a.memoizedState:null,n=e.memoizedState,a===null)if(n===null)if(e.stateNode===null)if(nt)e.stateNode=Km(e.type,e.memoizedProps,t.containerInfo,e);else{e:{t=e.type,n=e.memoizedProps,i=l.ownerDocument||l;t:switch(t){case"title":a=i.getElementsByTagName("title")[0],(!a||a[Ci]||a[st]||a.namespaceURI==="http://www.w3.org/2000/svg"||a.hasAttribute("itemprop"))&&(a=i.createElement(t),i.head.insertBefore(a,i.querySelector("head > title"))),_t(a,t,n),a[st]=e,et(a),t=a;break e;case"link":if(l=hf("link","href",i).get(t+(n.href||""))){for(s=0;s<l.length;s++)if(a=l[s],a.getAttribute("href")===(n.href==null||n.href===""?null:n.href)&&a.getAttribute("rel")===(n.rel==null?null:n.rel)&&a.getAttribute("title")===(n.title==null?null:n.title)&&a.getAttribute("crossorigin")===(n.crossOrigin==null?null:n.crossOrigin)){l.splice(s,1);break t}}a=i.createElement(t),_t(a,t,n),i.head.appendChild(a);break;case"meta":if(l=hf("meta","content",i).get(t+(n.content||""))){for(s=0;s<l.length;s++)if(a=l[s],a.getAttribute("content")===(n.content==null?null:""+n.content)&&a.getAttribute("name")===(n.name==null?null:n.name)&&a.getAttribute("property")===(n.property==null?null:n.property)&&a.getAttribute("http-equiv")===(n.httpEquiv==null?null:n.httpEquiv)&&a.getAttribute("charset")===(n.charSet==null?null:n.charSet)){l.splice(s,1);break t}}a=i.createElement(t),_t(a,t,n),i.head.appendChild(a);break;default:throw Error(d(468,t))}a[st]=e,et(a),t=a}e.stateNode=t}else nt||Dc(l,e.type,e.stateNode);else e.stateNode=yf(l,n,e.memoizedProps);else i!==n?(i===null?(t=a.stateNode,t===null||Ce||t.parentNode.removeChild(t)):i.count--,n===null?nt||Dc(l,e.type,e.stateNode):yf(l,n,e.memoizedProps)):n===null&&e.stateNode!==null&&Vo(e,e.memoizedProps,a.memoizedProps);break;case 27:yt(t,e,n),ht(e),i&512&&(Ce||a===null||dt(a,a.return)),a!==null&&i&4&&Vo(e,e.memoizedProps,a.memoizedProps);break;case 5:if(l=un,un=!1,yt(t,e,n),un=l,ht(e),i&512&&(Ce||a===null||dt(a,a.return)),e.flags&32){t=e.stateNode;try{Ua(t,""),Se=!0}catch(S){Oe(e,e.return,S)}}i&4&&e.stateNode!=null&&(t=e.memoizedProps,Vo(e,t,a!==null?a.memoizedProps:t)),i&1024&&(Ko=!0);break;case 6:if(yt(t,e,n),ht(e),i&4){if(e.stateNode===null)throw Error(d(162));t=e.memoizedProps,n=e.stateNode;try{n.nodeValue=t,Se=!0}catch(S){Oe(e,e.return,S)}}break;case 3:if(Se=!1,Ir=null,l=$t,$t=_l(t.containerInfo),yt(t,e,n),$t=l,ht(e),i&4&&a!==null&&a.memoizedState.isDehydrated)try{bi(t.containerInfo)}catch(S){Oe(e,e.return,S)}Ko&&(Ko=!1,dm(e)),Se=!1;break;case 4:i=un,un=nt,a=Ou(),l=$t,$t=_l(e.stateNode.containerInfo),yt(t,e,n),ht(e),$t=l,Se&&al&&(Er=!0),Se=a,un=i;break;case 12:yt(t,e,n),ht(e);break;case 31:yt(t,e,n),ht(e),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Tr(e,t)));break;case 13:yt(t,e,n),ht(e),e.child.flags&8192&&e.memoizedState!==null!=(a!==null&&a.memoizedState!==null)&&(Or=Ct()),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Tr(e,t)));break;case 22:l=e.memoizedState!==null,s=a!==null&&a.memoizedState!==null;var u=nt,_=Ce,h=un;nt=u||l,un=h||l,Ce=_||s,yt(t,e,n),Ce=_,un=h,nt=u,ht(e),i&8192&&(t=e.stateNode,t._visibility=l?t._visibility&-2:t._visibility|1,!l||a===null||s||nt||Ce||(t=s||Ce,n=nt,a=Ce,nt=l||nt,Ce=t,Xn(e,2),nt=n,Ce=a),!l&&un||Fo(e,l)),i&4&&(t=e.updateQueue,t!==null&&(n=t.retryQueue,n!==null&&(t.retryQueue=null,Tr(e,n))));break;case 19:yt(t,e,n),ht(e),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Tr(e,t)));break;case 30:i&512&&(Ce||a===null||dt(a,a.return)),i=Ou(),l=al,s=(n&335544064)===n,u=e.memoizedProps,al=s&&gn(u.default,u.update)!=="none",yt(t,e,n),ht(e),s&&a!==null&&Se&&(e.flags|=4),al=l,Se=i;break;case 21:break;case 7:i&512&&(Ce||a===null||dt(a,a.return)),a&&a.stateNode!==null&&(a.stateNode._fragmentFiber=e);default:yt(t,e,n),ht(e)}}function ht(e){var t=e.flags;if(t&2){try{for(var n,a=e.return;a!==null;){if(F_(a)){n=a;break}a=a.return}a=null;for(var i=e.return;i!==null;){if(Uo(i)){var l=i.stateNode;a===null?a=[l]:a.push(l)}if(zo(i))break;i=i.return}var s=a;if(n==null)throw Error(d(160));switch(n.tag){case 27:var u=n.stateNode,_=Lo(e);wr(e,_,u,s);break;case 5:var h=n.stateNode;n.flags&32&&(Ua(h,""),n.flags&=-33);var S=Lo(e);wr(e,S,h,s);break;case 3:case 4:var T=n.stateNode.containerInfo,v=Lo(e);Ho(e,v,T,s);break;default:throw Error(d(161))}}catch(w){Oe(e,e.return,w)}e.flags&=-3}t&4096&&(e.flags&=-4097)}function dm(e){if(e.subtreeFlags&1024)for(e=e.child;e!==null;){var t=e;dm(t),t.tag===5&&t.flags&1024&&(t=t.stateNode,gi=!0,t.reset(),gi=!1),e=e.sibling}}function ni(e,t){if(t.subtreeFlags&9270)for(t=t.child;t!==null;)_m(t,e),t=t.sibling;else nm(t)}function _m(e,t){var n=e.alternate;if(n===null)Go(e,!1);else switch(e.tag){case 3:if(Xo=dn=!1,$_(),ni(t,e),!dn&&!Er){if(e=on,e!==null)for(var a=0;a<e.length;a+=3){n=e[a];var i=e[a+1];Wm(n,e[a+2]),n=n.ownerDocument.documentElement,n!==null&&n.animate({opacity:[0,0],pointerEvents:["none","none"]},{duration:0,fill:"forwards",pseudoElement:"::view-transition-group("+i+")"})}e=t.containerInfo,e=e.nodeType===9?e.documentElement:e.ownerDocument.documentElement,e!==null&&e.style.viewTransitionName===""&&(e.style.viewTransitionName="none",e.animate({opacity:[0,0],pointerEvents:["none","none"]},{duration:0,fill:"forwards",pseudoElement:"::view-transition-group(root)"}),e.animate({width:[0,0],height:[0,0]},{duration:0,fill:"forwards",pseudoElement:"::view-transition"})),Xo=!0}on=null;break;case 5:ni(t,e);break;case 4:a=dn,dn=!1,ni(t,e),dn&&(Er=!0),dn=a;break;case 22:e.memoizedState===null&&(n.memoizedState!==null?Go(e,!1):ni(t,e));break;case 30:a=dn,i=$_(),dn=!1,ni(t,e),dn&&(e.flags|=4);var l=e.memoizedProps,s=e.stateNode;t=hn(l,s),s=hn(n.memoizedProps,s);var u=gn(l.default,l.update);u==="none"?t=!1:(l=n.memoizedState,n.memoizedState=null,n=e.child,jt=0,t=Po(e,n,t,s,u,l,!0),jt!==(l===null?0:l.length)&&(e.flags|=32)),(e.flags&4)!==0&&t?(ci(e,e.memoizedProps.onUpdate),on=i):i!==null&&(i.push.apply(i,on),on=i),dn=(e.flags&32)!==0?!0:a;break;default:ni(t,e)}}function _n(e,t){if(t.subtreeFlags&8772)for(t=t.child;t!==null;)im(e,t.alternate,t),t=t.sibling}function Xn(e,t){for(e=e.child;e!==null;){var n=e,a=t;switch(n.tag){case 0:case 11:case 14:case 15:Kn(4,n,n.return),Xn(n,a);break;case 1:dt(n,n.return);var i=n.stateNode;typeof i.componentWillUnmount=="function"&&K_(n,n.return,i),Xn(n,a);break;case 27:(a&2)!==0&&_f(n.stateNode,n.type,n.memoizedProps);case 5:dt(n,n.return),n.tag!==5&&n.tag!==27||nl(n),Xn(n,a);break;case 6:nl(n);break;case 26:dt(n,n.return),i=n.stateNode,n.memoizedState!==null||i===null||Ce||i.parentNode.removeChild(i),Xn(n,a);break;case 22:n.memoizedState===null&&Xn(n,a);break;case 30:dt(n,n.return),Xn(n,a);break;case 7:dt(n,n.return);default:Xn(n,a)}e=e.sibling}}function Wt(e,t,n){for(n=(t.subtreeFlags&8772)!==0?n:n&-2,t=t.child;t!==null;){var a=t.alternate,i=e,l=t,s=l.flags,u=(n&1)!==0;switch(l.tag){case 0:case 11:case 15:Wt(i,l,n),tl(4,l);break;case 1:if(Wt(i,l,n),a=l,i=a.stateNode,typeof i.componentDidMount=="function")try{i.componentDidMount()}catch(S){Oe(a,a.return,S)}if(a=l,i=a.updateQueue,i!==null){var _=a.stateNode;try{var h=i.shared.hiddenCallbacks;if(h!==null)for(i.shared.hiddenCallbacks=null,i=0;i<h.length;i++)Dd(h[i],_)}catch(S){Oe(a,a.return,S)}}u&&s&64&&P_(l),sn(l,l.return);break;case 27:(n&2)!==0&&Z_(l);case 5:l.tag!==5&&l.tag!==27||X_(l),Wt(i,l,n),u&&a===null&&s&4&&Bo(l),sn(l,l.return);break;case 6:X_(l);break;case 26:_=l.stateNode,l.memoizedState!==null||_===null||nt||Dc(_l(_.ownerDocument),l.type,_),Wt(i,l,n),u&&a===null&&s&4&&Bo(l),sn(l,l.return);break;case 12:Wt(i,l,n);break;case 31:Wt(i,l,n),u&&s&4&&om(i,l);break;case 13:Wt(i,l,n),u&&s&4&&cm(i,l);break;case 22:l.memoizedState===null&&Wt(i,l,n),sn(l,l.return);break;case 30:Wt(i,l,n),sn(l,l.return);break;case 7:sn(l,l.return);default:Wt(i,l,n)}t=t.sibling}}function Jo(e,t){var n=null;e!==null&&e.memoizedState!==null&&e.memoizedState.cachePool!==null&&(n=e.memoizedState.cachePool.pool),e=null,t.memoizedState!==null&&t.memoizedState.cachePool!==null&&(e=t.memoizedState.cachePool.pool),e!==n&&(e!=null&&e.refCount++,n!=null&&Gi(n))}function $o(e,t){e=null,t.alternate!==null&&(e=t.alternate.memoizedState.cache),t=t.memoizedState.cache,t!==e&&(t.refCount++,e!=null&&Gi(e))}function Pt(e,t,n,a){var i=(n&335544064)===n;if(t.subtreeFlags&(i?10262:10256))for(t=t.child;t!==null;)mm(e,t,n,a),t=t.sibling;else i&&tm(t)}function mm(e,t,n,a){var i=(n&335544064)===n;i&&t.alternate===null&&t.return!==null&&t.return.alternate!==null&&jr(t);var l=t.flags;switch(t.tag){case 0:case 11:case 15:Pt(e,t,n,a),l&2048&&tl(9,t);break;case 1:Pt(e,t,n,a);break;case 3:Pt(e,t,n,a),i&&Xo&&(e=e.containerInfo,e=e.nodeType===9?e.body:e.nodeName==="HTML"?e.ownerDocument.body:e,e.style.viewTransitionName==="root"&&(e.style.viewTransitionName=""),e=e.ownerDocument.documentElement,e!==null&&e.style.viewTransitionName==="none"&&(e.style.viewTransitionName="")),l&2048&&(l=null,t.alternate!==null&&(l=t.alternate.memoizedState.cache),t=t.memoizedState.cache,t!==l&&(t.refCount++,l!=null&&Gi(l)));break;case 12:if(l&2048){Pt(e,t,n,a),l=t.stateNode;try{var s=t.memoizedProps,u=s.id,_=s.onPostCommit;typeof _=="function"&&_(u,t.alternate===null?"mount":"update",l.passiveEffectDuration,-0)}catch(h){Oe(t,t.return,h)}}else Pt(e,t,n,a);break;case 31:Pt(e,t,n,a);break;case 13:Pt(e,t,n,a);break;case 23:break;case 22:s=t.stateNode,u=t.alternate,t.memoizedState!==null?(i&&u!==null&&u.memoizedState===null&&jr(u),s._visibility&2?Pt(e,t,n,a):il(e,t)):(i&&u!==null&&u.memoizedState!==null&&jr(t),s._visibility&2?Pt(e,t,n,a):(s._visibility|=2,ai(e,t,n,a,(t.subtreeFlags&10256)!==0||!1))),l&2048&&Jo(u,t);break;case 24:Pt(e,t,n,a),l&2048&&$o(t.alternate,t);break;case 30:i&&(l=t.alternate,l!==null&&(cn(l.child,!0),cn(t.child,!0))),Pt(e,t,n,a);break;default:Pt(e,t,n,a)}}function ai(e,t,n,a,i){for(i=i&&((t.subtreeFlags&10256)!==0||!1),t=t.child;t!==null;){var l=e,s=t,u=n,_=a,h=s.flags;switch(s.tag){case 0:case 11:case 15:ai(l,s,u,_,i),tl(8,s);break;case 23:break;case 22:var S=s.stateNode;s.memoizedState!==null?S._visibility&2?ai(l,s,u,_,i):il(l,s):(S._visibility|=2,ai(l,s,u,_,i)),i&&h&2048&&Jo(s.alternate,s);break;case 24:ai(l,s,u,_,i),i&&h&2048&&$o(s.alternate,s);break;default:ai(l,s,u,_,i)}t=t.sibling}}function il(e,t){if(t.subtreeFlags&10256)for(t=t.child;t!==null;){var n=e,a=t,i=a.flags;switch(a.tag){case 22:il(n,a),i&2048&&Jo(a.alternate,a);break;case 24:il(n,a),i&2048&&$o(a.alternate,a);break;default:il(n,a)}t=t.sibling}}var ja=8192;function Ea(e,t,n){if(e.subtreeFlags&ja)for(e=e.child;e!==null;)fm(e,t,n),e=e.sibling}function fm(e,t,n){switch(e.tag){case 26:Ea(e,t,n),e.flags&ja&&(e.memoizedState!==null?th(n,$t,e.memoizedState,e.memoizedProps):(e=e.stateNode,(t&335544128)===t&&wf(n,e)));break;case 5:Ea(e,t,n),e.flags&ja&&(e=e.stateNode,(t&335544128)===t&&wf(n,e));break;case 3:case 4:var a=$t;$t=_l(e.stateNode.containerInfo),Ea(e,t,n),$t=a;break;case 22:e.memoizedState===null&&(a=e.alternate,a!==null&&a.memoizedState!==null?(a=ja,ja=16777216,Ea(e,t,n),ja=a):Ea(e,t,n));break;case 30:if((e.flags&ja)!==0&&(a=e.memoizedProps.name,a!=null&&a!=="auto")){var i=e.stateNode;i.paired=null,Mt===null&&(Mt=new Map),Mt.set(a,i)}Ea(e,t,n);break;default:Ea(e,t,n)}}function pm(e){var t=e.alternate;if(t!==null&&(e=t.child,e!==null)){t.child=null;do t=e.sibling,e.sibling=null,e=t;while(e!==null)}}function ll(e){var t=e.deletions;if((e.flags&16)!==0){if(t!==null)for(var n=0;n<t.length;n++){var a=t[n];at=a,ym(a,e)}pm(e)}if(e.subtreeFlags&10256)for(e=e.child;e!==null;)vm(e),e=e.sibling}function vm(e){switch(e.tag){case 0:case 11:case 15:ll(e),e.flags&2048&&Kn(9,e,e.return);break;case 3:ll(e);break;case 12:ll(e);break;case 22:var t=e.stateNode;e.memoizedState!==null&&t._visibility&2&&(e.return===null||e.return.tag!==13)?(t._visibility&=-3,Cr(e)):ll(e);break;default:ll(e)}}function Cr(e){var t=e.deletions;if((e.flags&16)!==0){if(t!==null)for(var n=0;n<t.length;n++){var a=t[n];at=a,ym(a,e)}pm(e)}for(e=e.child;e!==null;){switch(t=e,t.tag){case 0:case 11:case 15:Kn(8,t,t.return),Cr(t);break;case 22:n=t.stateNode,n._visibility&2&&(n._visibility&=-3,Cr(t));break;default:Cr(t)}e=e.sibling}}function ym(e,t){for(;at!==null;){var n=at;switch(n.tag){case 0:case 11:case 15:Kn(8,n,t);break;case 23:case 22:if(n.memoizedState!==null&&n.memoizedState.cachePool!==null){var a=n.memoizedState.cachePool.pool;a!=null&&a.refCount++}break;case 24:Gi(n.memoizedState.cache)}if(a=n.child,a!==null)a.return=n,at=a;else e:for(n=e;at!==null;){a=at;var i=a.sibling,l=a.return;if(rm(a),a===n){at=null;break e}if(i!==null){i.return=l,at=i;break e}at=l}}}var Zv={getCacheForType:function(e){var t=ot(Pe),n=t.data.get(e);return n===void 0&&(n=e(),t.data.set(e,n)),n},cacheSignal:function(){return ot(Pe).controller.signal}},Jv=typeof WeakMap=="function"?WeakMap:Map,ke=0,ze=null,de=null,me=0,Ae=0,zt=null,Fn=!1,ii=!1,Wo=!1,Tn=0,Ie=0,Zn=0,ka=0,Ar=0,Ut=0,li=0,rl=null,kt=null,ec=!1,Or=0,hm=0,qr=1/0,Rr=null,Jn=null,He=0,en=null,Ta=null,mn=0,tc=0,nc=null,gm=null,ri=null,si=null,oi=null,sl=0,Dr=null;function Bt(){return(ke&2)!==0&&me!==0?me&-me:A.T!==null?_c():wu()}function bm(){if(Ut===0)if((me&536870912)===0||ue){var e=Tl;Tl<<=1,(Tl&3932160)===0&&(Tl=262144),Ut=e}else Ut=536870912;return e=ct.current,e!==null&&(e.flags|=32),Ut}function ci(e,t){if(t!=null){var n=e.stateNode,a=n.ref;a===null&&(a=n.ref=ef(hn(e.memoizedProps,n))),si===null&&(si=[]),si.push(t.bind(null,a))}}function Tt(e,t,n){(e===ze&&(Ae===2||Ae===9)||e.cancelPendingCommit!==null)&&(ui(e,0),$n(e,me,Ut,!1)),Ti(e,n),((ke&2)===0||e!==ze)&&(e===ze&&((ke&2)===0&&(ka|=n),Ie===4&&$n(e,me,Ut,!1)),fn(e))}function xm(e,t,n){if((ke&6)!==0)throw Error(d(327));var a=!n&&(t&127)===0&&(t&e.expiredLanes)===0||ki(e,t),i=a?ey(e,t):ic(e,t,!0),l=a;do{if(i===0){ii&&!a&&$n(e,t,0,!1);break}else{if(n=e.current.alternate,l&&!$v(n)){i=ic(e,t,!1),l=!1;continue}if(i===2){if(l=t,e.errorRecoveryDisabledLanes&l)var s=0;else s=e.pendingLanes&-536870913,s=s!==0?s:s&536870912?536870912:0;if(s!==0){t=s;e:{var u=e;i=rl;var _=u.current.memoizedState.isDehydrated;if(_&&(ui(u,s).flags|=256),s=ic(u,s,!1),s!==2&&s!==6){if(Wo&&!_){u.errorRecoveryDisabledLanes|=l,ka|=l,i=4;break e}l=kt,kt=i,l!==null&&(kt===null?kt=l:kt.push.apply(kt,l))}i=s}if(l=!1,i!==2)continue}}if(i===1){ui(e,0),$n(e,t,0,!0);break}e:{switch(a=e,l=i,l){case 0:case 1:throw Error(d(345));case 4:if((t&4194048)!==t&&(t&62914560)!==t)break;case 6:$n(a,t,Ut,!Fn);break e;case 2:kt=null;break;case 3:case 5:break;default:throw Error(d(329))}if((t&62914560)===t&&(i=Or+300-Ct(),10<i)){if($n(a,t,Ut,!Fn),Al(a,0,!0)!==0)break e;mn=t,a.timeoutHandle=Nc(wm.bind(null,a,n,kt,Rr,ec,t,Ut,ka,li,Fn,l,"Throttled",-0,0),i);break e}wm(a,n,kt,Rr,ec,t,Ut,ka,li,Fn,l,null,-0,0)}}break}while(!0);fn(e)}function wm(e,t,n,a,i,l,s,u,_,h,S,T,v,w){e.timeoutHandle=-1;var G=t.subtreeFlags,K=(l&335544064)===l;if(T=null,(K||G&8192||(G&16785408)===16785408)&&(T={stylesheets:null,count:0,imgCount:0,imgBytes:0,suspenseyImages:[],waitingForImages:!0,waitingForViewTransition:!1,unsuspend:an},Mt=null,fm(t,l,T),K&&(G=T,K=e.containerInfo,K=(K.nodeType===9?K:K.ownerDocument).__reactViewTransition,K!=null&&(G.count++,G.waitingForViewTransition=!0,G=pl.bind(G),K.finished.then(G,G))),G=(l&62914560)===l?Or-Ct():(l&4194048)===l?hm-Ct():0,G=nh(T,G),G!==null)){mn=l,e.cancelPendingCommit=G(Am.bind(null,e,t,l,n,a,i,s,u,_,h,S,T,null,v,w)),$n(e,l,s,!h);return}Am(e,t,l,n,a,i,s,u,_,h,S,T)}function $v(e){for(var t=e;;){var n=t.tag;if((n===0||n===11||n===15)&&t.flags&16384&&(n=t.updateQueue,n!==null&&(n=n.stores,n!==null)))for(var a=0;a<n.length;a++){var i=n[a],l=i.getSnapshot;i=i.value;try{if(!Rt(l(),i))return!1}catch{return!1}}if(n=t.child,t.subtreeFlags&16384&&n!==null)n.return=t,t=n;else{if(t===e)break;for(;t.sibling===null;){if(t.return===null||t.return===e)return!0;t=t.return}t.sibling.return=t.return,t=t.sibling}}return!0}function $n(e,t,n,a){t=yu(e,t),t&=~Ar,t&=~ka,e.suspendedLanes|=t,e.pingedLanes&=~t,a&&(e.warmLanes|=t),a=e.expirationTimes;for(var i=t;0<i;){var l=31-Ot(i),s=1<<l;a[l]=-1,i&=~s}n!==0&&gu(e,n,t)}function Mr(){return(ke&6)===0?(ol(0),!1):!0}function ac(){if(de!==null){if(Ae===0)var e=de.return;else e=de,wn=pa=null,uo(e),Za=null,Qi=0,e=de;for(;e!==null;)Q_(e.alternate,e),e=e.return;de=null}}function ui(e,t){var n=e.timeoutHandle;return n!==-1&&(e.timeoutHandle=-1,wy(n)),n=e.cancelPendingCommit,n!==null&&(e.cancelPendingCommit=null,n()),mn=0,ac(),ze=e,de=n=bn(e.current,null),me=t,Ae=0,zt=null,Fn=!1,ii=ki(e,t),Wo=!1,li=Ut=Ar=ka=Zn=Ie=0,kt=rl=null,ec=!1,Tn=yu(e,t),Il(),n}function Nm(e,t){se=null,A.H=fr,t===Fa||t===er?(t=Ad(),Ae=3):t===Js?(t=Ad(),Ae=4):Ae=t===Eo?8:t!==null&&typeof t=="object"&&typeof t.then=="function"?6:1,zt=t,de===null&&(Ie=1,pr(e,Gt(t,e.current)))}function Sm(){var e=ct.current;return e===null?!0:(me&4194048)===me?ft===null:(me&62914560)===me||(me&536870912)!==0?e===ft:!1}function jm(){var e=A.H;return A.H=fr,e===null?fr:e}function Em(){var e=A.A;return A.A=Zv,e}function zr(){Ie=4,Fn||(me&4194048)!==me&&ct.current!==null||(ii=!0),(Zn&134217727)===0&&(ka&134217727)===0||ze===null||$n(ze,me,Ut,!1)}function ic(e,t,n){var a=ke;ke|=2;var i=jm(),l=Em();(ze!==e||me!==t)&&(Rr=null,ui(e,t)),t=!1;var s=Ie;e:do try{if(Ae!==0&&de!==null){var u=de,_=zt;switch(Ae){case 8:ac(),s=6;break e;case 3:case 2:case 9:case 6:ct.current===null&&(t=!0);var h=Ae;if(Ae=0,zt=null,di(e,u,_,h),n&&ii){s=0;break e}break;default:h=Ae,Ae=0,zt=null,di(e,u,_,h)}}Wv(),s=Ie;break}catch(S){Nm(e,S)}while(!0);return t&&e.shellSuspendCounter++,wn=pa=null,ke=a,A.H=i,A.A=l,de===null&&(ze=null,me=0,Il()),s}function Wv(){for(;de!==null;)km(de)}function ey(e,t){var n=ke;ke|=2;var a=jm(),i=Em();ze!==e||me!==t?(Rr=null,qr=Ct()+500,ui(e,t)):ii=ki(e,t);e:do try{if(Ae!==0&&de!==null){t=de;var l=zt;t:switch(Ae){case 1:Ae=0,zt=null,di(e,t,l,1);break;case 2:case 9:if(Td(l)){Ae=0,zt=null,Tm(t);break}t=function(){Ae!==2&&Ae!==9||ze!==e||(Ae=7),fn(e)},l.then(t,t);break e;case 3:Ae=7;break e;case 4:Ae=5;break e;case 7:Td(l)?(Ae=0,zt=null,Tm(t)):(Ae=0,zt=null,di(e,t,l,7));break;case 5:var s=null;switch(de.tag){case 26:s=de.memoizedState;case 5:case 27:var u=de;if(s?bf(s):u.stateNode.complete){Ae=0,zt=null;var _=u.sibling;if(_!==null)de=_;else{var h=u.return;h!==null?(de=h,Ur(h)):de=null}break t}}Ae=0,zt=null,di(e,t,l,5);break;case 6:Ae=0,zt=null,di(e,t,l,6);break;case 8:ac(),Ie=6;break e;default:throw Error(d(462))}}ty();break}catch(S){Nm(e,S)}while(!0);return wn=pa=null,A.H=a,A.A=i,ke=n,de!==null?0:(ze=null,me=0,Il(),Ie)}function ty(){for(;de!==null&&!hp();)km(de)}function km(e){var t=I_(e.alternate,e,Tn);e.memoizedProps=e.pendingProps,t===null?Ur(e):de=t}function Tm(e){var t=e,n=t.alternate;switch(t.tag){case 15:case 0:t=z_(n,t,t.pendingProps,t.type,void 0,me);break;case 11:t=z_(n,t,t.pendingProps,t.type.render,t.ref,me);break;case 5:uo(t);var a=t;a===tt&&(ue?(Fl(a),a.tag===5&&a.stateNode!=null&&(Ue=a.stateNode)):(Fl(a),ue=!0));default:Q_(n,t),t=de=yd(t,Tn),t=I_(n,t,Tn)}e.memoizedProps=e.pendingProps,t===null?Ur(e):de=t}function di(e,t,n,a){wn=pa=null,uo(t),Za=null,Qi=0;var i=t.return;try{if(Gv(e,i,t,n,me)){Ie=1,pr(e,Gt(n,e.current)),de=null;return}}catch(l){if(i!==null)throw de=i,l;Ie=1,pr(e,Gt(n,e.current)),de=null;return}t.flags&32768?(ue||a===1?e=!0:ii||(me&536870912)!==0?e=!1:(Fn=e=!0,(a===2||a===9||a===3||a===6)&&(a=ct.current,a!==null&&a.tag===13&&(a.flags|=16384))),Cm(t,e)):Ur(t)}function Ur(e){var t=e;do{if((t.flags&32768)!==0){Cm(t,Fn);return}e=t.return;var n=Pv(t.alternate,t,Tn);if(n!==null){de=n;return}if(t=t.sibling,t!==null){de=t;return}de=t=e}while(t!==null);Ie===0&&(Ie=5)}function Cm(e,t){do{var n=Kv(e.alternate,e);if(n!==null){n.flags&=32767,de=n;return}if(n=e.return,n!==null&&(n.flags|=32768,n.subtreeFlags=0,n.deletions=null),!t&&(e=e.sibling,e!==null)){de=e;return}de=e=n}while(e!==null);Ie=6,de=null}function Am(e,t,n,a,i,l,s,u,_,h,S,T){e.cancelPendingCommit=null;do Br();while(He!==0);if((ke&6)!==0)throw Error(d(327));if(t!==null){if(t===e.current)throw Error(d(177));e===ze&&(de=ze=null,me=0),Ta=t,en=e,mn=n,nc=i,gm=a,ny(e,t,n,s,u,_,T)}}function ny(e,t,n,a,i,l,s){var u=t.lanes|t.childLanes;if(tc=u,u|=Bs,Tp(e,n,u,a,i,l),si=null,(n&335544064)===n?(oi=Av(e),a=10262):(oi=null,a=10256),(t.subtreeFlags&a)!==0||(t.flags&a)!==0?(e.callbackNode=null,e.callbackPriority=0,oy(El,function(){return oc(),null})):(e.callbackNode=null,e.callbackPriority=0),Nr=!1,a=(t.flags&13878)!==0,(t.subtreeFlags&13878)!==0||a){a=A.T,A.T=null,i=U.p,U.p=2,l=ke,ke|=4;try{Xv(e,t,n)}finally{ke=l,U.p=i,A.T=a}}He=1,Nr?ri=Ty(s,e.containerInfo,oi,lc,rc,iy,sc,oc,ay):(lc(),rc(),sc())}function ay(e){if(He!==0){var t=en.onRecoverableError;t(e,{componentStack:null})}}function iy(){He===3&&(He=0,_m(Ta,en),He=4)}function lc(){if(He===1){He=0;var e=en,t=Ta,n=mn,a=(t.flags&13878)!==0;if((t.subtreeFlags&13878)!==0||a){a=A.T,A.T=null;var i=U.p;U.p=2;var l=ke;ke|=4;try{al=Er=!1,um(t,e,n),n=bc;var s=sd(e.containerInfo),u=n.focusedElem,_=n.selectionRange;if(s!==u&&u&&u.ownerDocument&&rd(u.ownerDocument.documentElement,u)){if(_!==null&&Rs(u)){var h=_.start,S=_.end;if(S===void 0&&(S=h),"selectionStart"in u)u.selectionStart=h,u.selectionEnd=Math.min(S,u.value.length);else{var T=u.ownerDocument||document,v=T&&T.defaultView||window;if(v.getSelection){var w=v.getSelection(),G=u.textContent.length,K=Math.min(_.start,G),oe=_.end===void 0?K:Math.min(_.end,G);!w.extend&&K>oe&&(s=oe,oe=K,K=s);var y=ld(u,K),f=ld(u,oe);if(y&&f&&(w.rangeCount!==1||w.anchorNode!==y.node||w.anchorOffset!==y.offset||w.focusNode!==f.node||w.focusOffset!==f.offset)){var b=T.createRange();b.setStart(y.node,y.offset),w.removeAllRanges(),K>oe?(w.addRange(b),w.extend(f.node,f.offset)):(b.setEnd(f.node,f.offset),w.addRange(b))}}}}for(T=[],w=u;w=w.parentNode;)w.nodeType===1&&T.push({element:w,left:w.scrollLeft,top:w.scrollTop});for(typeof u.focus=="function"&&u.focus(),u=0;u<T.length;u++){var k=T[u];k.element.scrollLeft=k.left,k.element.scrollTop=k.top}}gi=!!gc,bc=gc=null}finally{ke=l,U.p=i,A.T=a}}e.current=t,He=2}}function rc(){if(He===2){He=0;var e=en,t=Ta,n=(t.flags&8772)!==0;if((t.subtreeFlags&8772)!==0||n){n=A.T,A.T=null;var a=U.p;U.p=2;var i=ke;ke|=4;try{im(e,t.alternate,t)}finally{ke=i,U.p=a,A.T=n}}He=3}}function sc(){if(He===4||He===3){He=0;var e=ri;ri=null,gp();var t=en,n=Ta,a=mn,i=gm,l=(a&335544064)===a?10262:10256;if((n.subtreeFlags&l)!==0||(n.flags&l)!==0?He=5:(He=0,Ta=en=null,Om(t,t.pendingLanes)),l=t.pendingLanes,l===0&&(Jn=null),ps(a),n=n.stateNode,At&&typeof At.onCommitFiberRoot=="function")try{At.onCommitFiberRoot(Ei,n,void 0,(n.current.flags&128)===128)}catch{}if(i!==null){n=A.T,l=U.p,U.p=2,A.T=null;try{for(var s=t.onRecoverableError,u=0;u<i.length;u++){var _=i[u];s(_.value,{componentStack:_.stack})}}finally{A.T=n,U.p=l}}if(i=si,s=oi,oi=null,i!==null&&(si=null,s===null&&(s=[]),e!==null))for(_=0;_<i.length;_++)n=(0,i[_])(s),n!==void 0&&e.finished.finally(n);(mn&3)!==0&&Br(),fn(t),l=t.pendingLanes,(a&261930)!==0&&(l&42)!==0?t===Dr?sl++:(sl=0,Dr=t):(sl=0,Dr=null),ol(0)}}function Om(e,t){(e.pooledCacheLanes&=t)===0&&(t=e.pooledCache,t!=null&&(e.pooledCache=null,Gi(t)))}function Br(){return ri!==null&&(ri.skipTransition(),ri=null),lc(),rc(),sc(),oc()}function oc(){if(He!==5)return!1;var e=en,t=tc;tc=0;var n=ps(mn),a=A.T,i=U.p;try{U.p=32>n?32:n,A.T=null,n=nc,nc=null;var l=en,s=mn;if(He=0,Ta=en=null,mn=0,(ke&6)!==0)throw Error(d(331));var u=ke;if(ke|=4,vm(l.current),mm(l,l.current,s,n),ke=u,ol(0,!1),At&&typeof At.onPostCommitFiberRoot=="function")try{At.onPostCommitFiberRoot(Ei,l)}catch{}return!0}finally{U.p=i,A.T=a,Om(e,t)}}function qm(e,t,n){t=Gt(n,t),t=jo(e.stateNode,t,2),e=In(e,t,2),e!==null&&(Ti(e,2),fn(e))}function Oe(e,t,n){if(e.tag===3)qm(e,e,n);else for(;t!==null;){if(t.tag===3){qm(t,e,n);break}else if(t.tag===1){var a=t.stateNode;if(typeof t.type.getDerivedStateFromError=="function"||typeof a.componentDidCatch=="function"&&(Jn===null||!Jn.has(a))){e=Gt(n,e),n=T_(2),a=In(t,n,2),a!==null&&(C_(n,a,t,e),Ti(a,2),fn(a));break}}t=t.return}}function cc(e,t,n){var a=e.pingCache;if(a===null){a=e.pingCache=new Jv;var i=new Set;a.set(t,i)}else i=a.get(t),i===void 0&&(i=new Set,a.set(t,i));i.has(n)||(Wo=!0,i.add(n),e=ly.bind(null,e,t,n),t.then(e,e))}function ly(e,t,n){var a=e.pingCache;a!==null&&a.delete(t),e.pingedLanes|=e.suspendedLanes&n,e.warmLanes&=~n,ze===e&&(me&n)===n&&((Ie===4||Ie===3&&(me&62914560)===me&&300>Ct()-Or)&&(ke&2)===0?ui(e,0):Ar|=n,li===me&&(li=0)),fn(e)}function Rm(e,t){t===0&&(t=hu()),e=_a(e,t),e!==null&&(Ti(e,t),fn(e))}function ry(e){var t=e.memoizedState,n=0;t!==null&&(n=t.retryLane),Rm(e,n)}function sy(e,t){var n=0;switch(e.tag){case 31:case 13:var a=e.stateNode,i=e.memoizedState;i!==null&&(n=i.retryLane);break;case 19:a=e.stateNode;break;case 22:a=e.stateNode._retryCache;break;default:throw Error(d(314))}a!==null&&a.delete(t),Rm(e,n)}function oy(e,t){return ds(e,t)}var _i=null,mi=null,uc=!1,Vr=!1,dc=!1,Wn=0;function fn(e){e!==mi&&e.next===null&&(mi===null?_i=mi=e:mi=mi.next=e),Vr=!0,uc||(uc=!0,uy())}function ol(e,t){if(!dc&&Vr){dc=!0;do for(var n=!1,a=_i;a!==null;){if(e!==0){var i=a.pendingLanes;if(i===0)var l=0;else{var s=a.suspendedLanes,u=a.pingedLanes;l=(1<<31-Ot(42|e)+1)-1,l&=i&~(s&~u),l=l&201326741?l&201326741|1:l?l|2:0}l!==0&&(n=!0,Um(a,l))}else l=me,l=Al(a,a===ze?l:0,a.cancelPendingCommit!==null||a.timeoutHandle!==-1),(l&3)===0||ki(a,l)||(n=!0,Um(a,l));a=a.next}while(n);dc=!1}}function cy(){Dm()}function Dm(){Vr=uc=!1;var e=0;Wn!==0&&xy()&&(e=Wn);for(var t=Ct(),n=null,a=_i;a!==null;){var i=a.next,l=Mm(a,t);l===0?(a.next=null,n===null?_i=i:n.next=i,i===null&&(mi=n)):(n=a,(e!==0||(l&3)!==0)&&(Vr=!0)),a=i}He!==0&&He!==5||ol(e),Wn!==0&&(Wn=0)}function Mm(e,t){for(var n=e.suspendedLanes,a=e.pingedLanes,i=e.expirationTimes,l=e.pendingLanes&-62914561;0<l;){var s=31-Ot(l),u=1<<s,_=i[s];_===-1?((u&n)===0||(u&a)!==0)&&(i[s]=kp(u,t)):_<=t&&(e.expiredLanes|=u),l&=~u}if(t=ze,n=me,n=Al(e,e===t?n:0,e.cancelPendingCommit!==null||e.timeoutHandle!==-1),a=e.callbackNode,n===0||e===t&&(Ae===2||Ae===9)||e.cancelPendingCommit!==null)return a!==null&&a!==null&&_s(a),e.callbackNode=null,e.callbackPriority=0;if((n&3)===0||ki(e,n)){if(t=n&-n,t===e.callbackPriority)return t;switch(a!==null&&_s(a),ps(n)){case 2:case 8:n=pu;break;case 32:n=El;break;case 268435456:n=vu;break;default:n=El}return a=zm.bind(null,e),n=ds(n,a),e.callbackPriority=t,e.callbackNode=n,t}return a!==null&&a!==null&&_s(a),e.callbackPriority=2,e.callbackNode=null,2}function zm(e,t){if(He!==0&&He!==5)return e.callbackNode=null,e.callbackPriority=0,null;var n=e.callbackNode;if(Br()&&e.callbackNode!==n)return null;var a=me;return a=Al(e,e===ze?a:0,e.cancelPendingCommit!==null||e.timeoutHandle!==-1),a===0?null:(xm(e,a,t),Mm(e,Ct()),e.callbackNode!=null&&e.callbackNode===n?zm.bind(null,e):null)}function Um(e,t){if(Br())return null;xm(e,t,!0)}function uy(){Ny(function(){(ke&6)!==0?ds(fu,cy):Dm()})}function _c(){if(Wn===0){var e=ha;e===0&&(e=kl,kl<<=1,(kl&261888)===0&&(kl=256)),Wn=e}return Wn}function Bm(e){return e==null||typeof e=="symbol"||typeof e=="boolean"?null:typeof e=="function"?e:Ml(e)}function dy(e,t,n,a,i){if(t==="submit"&&n&&n.stateNode===i){var l=Bm((i[Nt]||null).action),s=a.submitter;s&&(t=(t=s[Nt]||null)?Bm(t.formAction):s.getAttribute("formAction"),t!==null&&(l=t,s=null));var u=new Vl("action","action",null,a,i);e.push({event:u,listeners:[{instance:null,listener:function(){if(a.defaultPrevented){if(Wn!==0){var _=new FormData(i,s);bo(n,{pending:!0,data:_,method:i.method,action:l},null,_)}}else typeof l=="function"&&(u.preventDefault(),_=new FormData(i,s),bo(n,{pending:!0,data:_,method:i.method,action:l},l,_))},currentTarget:i}]})}}for(var mc=0;mc<Us.length;mc++){var fc=Us[mc],_y=fc.toLowerCase(),my=fc[0].toUpperCase()+fc.slice(1);Zt(_y,"on"+my)}Zt(ud,"onAnimationEnd"),Zt(dd,"onAnimationIteration"),Zt(_d,"onAnimationStart"),Zt("dblclick","onDoubleClick"),Zt("focusin","onFocus"),Zt("focusout","onBlur"),Zt(wv,"onTransitionRun"),Zt(Nv,"onTransitionStart"),Zt(Sv,"onTransitionCancel"),Zt(md,"onTransitionEnd"),Ma("onMouseEnter",["mouseout","mouseover"]),Ma("onMouseLeave",["mouseout","mouseover"]),Ma("onPointerEnter",["pointerout","pointerover"]),Ma("onPointerLeave",["pointerout","pointerover"]),ca("onChange","change click focusin focusout input keydown keyup selectionchange".split(" ")),ca("onSelect","focusout contextmenu dragend focusin keydown keyup mousedown mouseup selectionchange".split(" ")),ca("onBeforeInput",["compositionend","keypress","textInput","paste"]),ca("onCompositionEnd","compositionend focusout keydown keypress keyup mousedown".split(" ")),ca("onCompositionStart","compositionstart focusout keydown keypress keyup mousedown".split(" ")),ca("onCompositionUpdate","compositionupdate focusout keydown keypress keyup mousedown".split(" "));var cl="abort canplay canplaythrough durationchange emptied encrypted ended error loadeddata loadedmetadata loadstart pause play playing progress ratechange resize seeked seeking stalled suspend timeupdate volumechange waiting".split(" "),fy=new Set("beforetoggle cancel close invalid load scroll scrollend toggle".split(" ").concat(cl));function Vm(e,t){t=(t&4)!==0;for(var n=0;n<e.length;n++){var a=e[n],i=a.event;a=a.listeners;e:{var l=void 0;if(t)for(var s=a.length-1;0<=s;s--){var u=a[s],_=u.instance,h=u.currentTarget;if(u=u.listener,_!==l&&i.isPropagationStopped())break e;l=u,i.currentTarget=h;try{l(i)}catch(S){Gl(S)}i.currentTarget=null,l=_}else for(s=0;s<a.length;s++){if(u=a[s],_=u.instance,h=u.currentTarget,u=u.listener,_!==l&&i.isPropagationStopped())break e;l=u,i.currentTarget=h;try{l(i)}catch(S){Gl(S)}i.currentTarget=null,l=_}}}}function _e(e,t){var n=t[Su];n===void 0&&(n=t[Su]=new Set);var a=e+"__bubble";n.has(a)||(Lm(t,e,2,!1),n.add(a))}function pc(e,t,n){var a=0;t&&(a|=4),Lm(n,e,a,t)}var Lr="_reactListening"+Math.random().toString(36).slice(2);function vc(e){if(!e[Lr]){e[Lr]=!0,ku.forEach(function(n){n!=="selectionchange"&&(fy.has(n)||pc(n,!1,e),pc(n,!0,e))});var t=e.nodeType===9?e:e.ownerDocument;t===null||t[Lr]||(t[Lr]=!0,pc("selectionchange",!1,t))}}function Lm(e,t,n,a){switch(Af(t)){case 2:var i=rh;break;case 8:i=sh;break;default:i=zc}n=i.bind(null,t,n,e),i=void 0,!Ns||t!=="touchstart"&&t!=="touchmove"&&t!=="wheel"||(i=!0),a?i!==void 0?e.addEventListener(t,n,{capture:!0,passive:i}):e.addEventListener(t,n,!0):i!==void 0?e.addEventListener(t,n,{passive:i}):e.addEventListener(t,n,!1)}function yc(e,t,n,a,i){var l=a;if((t&1)===0&&(t&2)===0&&a!==null)e:for(;;){if(a===null)return;var s=a.tag;if(s===3||s===4){var u=a.stateNode.containerInfo;if(u===i)break;if(s===4)for(s=a.return;s!==null;){var _=s.tag;if((_===3||_===4)&&s.stateNode.containerInfo===i)return;s=s.return}for(;u!==null;){if(s=oa(u),s===null)return;if(_=s.tag,_===5||_===6||_===26||_===27){a=l=s;continue e}u=u.parentNode}}a=a.return}Lu(function(){var h=l,S=xs(n),T=[];e:{var v=fd.get(e);if(v!==void 0){var w=Vl,G=e;switch(e){case"keypress":if(Ul(n)===0)break e;case"keydown":case"keyup":w=$p;break;case"focusin":G="focus",w=ks;break;case"focusout":G="blur",w=ks;break;case"beforeblur":case"afterblur":w=ks;break;case"click":if(n.button===2)break e;case"auxclick":case"dblclick":case"mousedown":case"mousemove":case"mouseup":case"mouseout":case"mouseover":case"contextmenu":w=Iu;break;case"drag":case"dragend":case"dragenter":case"dragexit":case"dragleave":case"dragover":case"dragstart":case"drop":w=Lp;break;case"touchcancel":case"touchend":case"touchmove":case"touchstart":w=av;break;case ud:case dd:case _d:w=Ip;break;case md:w=lv;break;case"scroll":case"scrollend":w=Bp;break;case"wheel":w=sv;break;case"copy":case"cut":case"paste":w=Qp;break;case"gotpointercapture":case"lostpointercapture":case"pointercancel":case"pointerdown":case"pointermove":case"pointerout":case"pointerover":case"pointerup":w=Qu;break;case"submit":w=tv;break;case"toggle":case"beforetoggle":w=cv}var K=(t&4)!==0,oe=!K&&(e==="scroll"||e==="scrollend"),y=K?v!==null?v+"Capture":null:v;K=[];for(var f=h,b;f!==null;){var k=f;if(b=k.stateNode,k=k.tag,k!==5&&k!==26&&k!==27||b===null||y===null||(k=Oi(f,y),k!=null&&K.push(ul(f,k,b))),oe)break;f=f.return}0<K.length&&(v=new w(v,G,null,n,S),T.push({event:v,listeners:K}))}}if((t&7)===0){e:{if(w=e==="mouseover"||e==="pointerover",v=e==="mouseout"||e==="pointerout",w&&n!==bs&&(G=n.relatedTarget||n.fromElement)&&(oa(G)||G[qa]))break e;(v||w)&&(G=S.window===S?S:(w=S.ownerDocument)?w.defaultView||w.parentWindow:window,v?(w=n.relatedTarget||n.toElement,v=h,w=w?oa(w):null,w!==null&&(oe=O(w),K=w.tag,w!==oe||K!==5&&K!==27&&K!==6)&&(w=null)):(v=null,w=h),v!==w&&(K=Iu,k="onMouseLeave",y="onMouseEnter",f="mouse",(e==="pointerout"||e==="pointerover")&&(K=Qu,k="onPointerLeave",y="onPointerEnter",f="pointer"),oe=v==null?G:Ai(v),b=w==null?G:Ai(w),G=new K(k,f+"leave",v,n,S),G.target=oe,G.relatedTarget=b,k=null,oa(S)===h&&(K=new K(y,f+"enter",w,n,S),K.target=b,K.relatedTarget=oe,k=K),oe=k,K=v&&w?C(v,w,py):null,v!==null&&Hm(T,G,v,K,!1),w!==null&&oe!==null&&Hm(T,oe,w,K,!0)))}e:{if(v=h?Ai(h):window,w=v.nodeName&&v.nodeName.toLowerCase(),w==="select"||w==="input"&&v.type==="file")var Y=Wu;else if(Ju(v))if(ed)Y=gv;else{Y=yv;var fe=vv}else w=v.nodeName,!w||w.toLowerCase()!=="input"||v.type!=="checkbox"&&v.type!=="radio"?h&&gs(h.elementType)&&(Y=Wu):Y=hv;if(Y&&(Y=Y(e,h))){$u(T,Y,n,S);break e}fe&&fe(e,v,h)}switch(fe=h?Ai(h):window,e){case"focusin":(Ju(fe)||fe.contentEditable==="true")&&(Ha=fe,Ds=h,Vi=null);break;case"focusout":Vi=Ds=Ha=null;break;case"mousedown":Ms=!0;break;case"contextmenu":case"mouseup":case"dragend":Ms=!1,od(T,n,S);break;case"selectionchange":if(xv)break;case"keydown":case"keyup":od(T,n,S)}var te;if(Cs)e:{switch(e){case"compositionstart":var ie="onCompositionStart";break e;case"compositionend":ie="onCompositionEnd";break e;case"compositionupdate":ie="onCompositionUpdate";break e}ie=void 0}else La?Fu(e,n)&&(ie="onCompositionEnd"):e==="keydown"&&n.keyCode===229&&(ie="onCompositionStart");ie&&(Pu&&n.locale!=="ko"&&(La||ie!=="onCompositionStart"?ie==="onCompositionEnd"&&La&&(te=Hu()):(Dn=S,Ss="value"in Dn?Dn.value:Dn.textContent,La=!0)),fe=Hr(h,ie),0<fe.length&&(ie=new Yu(ie,e,null,n,S),T.push({event:ie,listeners:fe}),te?ie.data=te:(te=Zu(n),te!==null&&(ie.data=te)))),(te=dv?_v(e,n):mv(e,n))&&(ie=Hr(h,"onBeforeInput"),0<ie.length&&(fe=new Yu("onBeforeInput","beforeinput",null,n,S),T.push({event:fe,listeners:ie}),fe.data=te)),dy(T,e,h,n,S)}Vm(T,t)})}function ul(e,t,n){return{instance:e,listener:t,currentTarget:n}}function Hr(e,t){for(var n=t+"Capture",a=[];e!==null;){var i=e,l=i.stateNode;if(i=i.tag,i!==5&&i!==26&&i!==27||l===null||(i=Oi(e,n),i!=null&&a.unshift(ul(e,i,l)),i=Oi(e,t),i!=null&&a.push(ul(e,i,l))),e.tag===3)return a;e=e.return}return[]}function py(e){if(e===null)return null;do e=e.return;while(e&&e.tag!==5&&e.tag!==27);return e||null}function Hm(e,t,n,a,i){for(var l=t._reactName,s=[];n!==null&&n!==a;){var u=n,_=u.alternate,h=u.stateNode;if(u=u.tag,_!==null&&_===a)break;u!==5&&u!==26&&u!==27||h===null||(_=h,i?(h=Oi(n,l),h!=null&&s.unshift(ul(n,h,_))):i||(h=Oi(n,l),h!=null&&s.push(ul(n,h,_)))),n=n.return}s.length!==0&&e.push({event:t,listeners:s})}var vy=/\r\n?/g,yy=/\u0000|\uFFFD/g;function Gm(e){return(typeof e=="string"?e:""+e).replace(vy,`
`).replace(yy,"")}function Im(e,t){return t=Gm(t),Gm(e)===t}function qe(e,t,n,a,i,l){switch(n){case"children":if(typeof a=="string")t==="body"||t==="textarea"&&a===""||Ua(e,a);else if(typeof a=="number"||typeof a=="bigint")t!=="body"&&Ua(e,""+a);else return;break;case"className":Dl(e,"class",a);break;case"tabIndex":Dl(e,"tabindex",a);break;case"dir":case"role":case"viewBox":case"width":case"height":Dl(e,n,a);break;case"style":Bu(e,a,l);return;case"data":if(t!=="object"){Dl(e,"data",a);break}case"src":case"href":if(a===""&&(t!=="a"||n!=="href")){e.removeAttribute(n);break}if(a==null||typeof a=="function"||typeof a=="symbol"||typeof a=="boolean"){e.removeAttribute(n);break}a=Ml(a),e.setAttribute(n,a);break;case"action":case"formAction":if(typeof a=="function"){e.setAttribute(n,"javascript:throw new Error('A React form was unexpectedly submitted. If you called form.submit() manually, consider using form.requestSubmit() instead. If you\\'re trying to use event.stopPropagation() in a submit event handler, consider also calling event.preventDefault().')");break}else typeof l=="function"&&(n==="formAction"?(t!=="input"&&qe(e,t,"name",i.name,i,null),qe(e,t,"formEncType",i.formEncType,i,null),qe(e,t,"formMethod",i.formMethod,i,null),qe(e,t,"formTarget",i.formTarget,i,null)):(qe(e,t,"encType",i.encType,i,null),qe(e,t,"method",i.method,i,null),qe(e,t,"target",i.target,i,null)));if(a==null||typeof a=="symbol"||typeof a=="boolean"){e.removeAttribute(n);break}a=Ml(a),e.setAttribute(n,a);break;case"onClick":a!=null&&(e.onclick=an);return;case"onScroll":a!=null&&_e("scroll",e);return;case"onScrollEnd":a!=null&&_e("scrollend",e);return;case"dangerouslySetInnerHTML":if(a!=null){if(typeof a!="object"||!("__html"in a))throw Error(d(61));if(n=a.__html,n!=null){if(i.children!=null)throw Error(d(60));(l!=null?l.__html:void 0)!==n&&(e.innerHTML=n)}}break;case"multiple":e.multiple=a&&typeof a!="function"&&typeof a!="symbol";break;case"muted":e.muted=a&&typeof a!="function"&&typeof a!="symbol";break;case"suppressContentEditableWarning":case"suppressHydrationWarning":case"defaultValue":case"defaultChecked":case"innerHTML":case"ref":break;case"autoFocus":break;case"xlinkHref":if(a==null||typeof a=="function"||typeof a=="boolean"||typeof a=="symbol"){e.removeAttribute("xlink:href");break}n=Ml(a),e.setAttributeNS("http://www.w3.org/1999/xlink","xlink:href",n);break;case"contentEditable":case"spellCheck":case"draggable":case"value":case"autoReverse":case"externalResourcesRequired":case"focusable":case"preserveAlpha":a!=null&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,a):e.removeAttribute(n);break;case"inert":case"allowFullScreen":case"async":case"autoPlay":case"controls":case"credentialless":case"default":case"defer":case"disabled":case"disablePictureInPicture":case"disableRemotePlayback":case"formNoValidate":case"hidden":case"loop":case"noModule":case"noValidate":case"open":case"playsInline":case"readOnly":case"required":case"reversed":case"scoped":case"seamless":case"itemScope":a&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,""):e.removeAttribute(n);break;case"capture":case"download":a===!0?e.setAttribute(n,""):a!==!1&&a!=null&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,a):e.removeAttribute(n);break;case"cols":case"rows":case"size":case"span":a!=null&&typeof a!="function"&&typeof a!="symbol"&&!isNaN(a)&&1<=a?e.setAttribute(n,a):e.removeAttribute(n);break;case"rowSpan":case"start":a==null||typeof a=="function"||typeof a=="symbol"||isNaN(a)?e.removeAttribute(n):e.setAttribute(n,a);break;case"popover":_e("beforetoggle",e),_e("toggle",e),Rl(e,"popover",a);break;case"xlinkActuate":vn(e,"http://www.w3.org/1999/xlink","xlink:actuate",a);break;case"xlinkArcrole":vn(e,"http://www.w3.org/1999/xlink","xlink:arcrole",a);break;case"xlinkRole":vn(e,"http://www.w3.org/1999/xlink","xlink:role",a);break;case"xlinkShow":vn(e,"http://www.w3.org/1999/xlink","xlink:show",a);break;case"xlinkTitle":vn(e,"http://www.w3.org/1999/xlink","xlink:title",a);break;case"xlinkType":vn(e,"http://www.w3.org/1999/xlink","xlink:type",a);break;case"xmlBase":vn(e,"http://www.w3.org/XML/1998/namespace","xml:base",a);break;case"xmlLang":vn(e,"http://www.w3.org/XML/1998/namespace","xml:lang",a);break;case"xmlSpace":vn(e,"http://www.w3.org/XML/1998/namespace","xml:space",a);break;case"is":Rl(e,"is",a);break;case"innerText":case"textContent":return;default:if(!(2<n.length)||n[0]!=="o"&&n[0]!=="O"||n[1]!=="n"&&n[1]!=="N")n=zp.get(n)||n,Rl(e,n,a);else return}Se=!0}function hc(e,t,n,a,i,l){switch(n){case"style":Bu(e,a,l);return;case"dangerouslySetInnerHTML":if(a!=null){if(typeof a!="object"||!("__html"in a))throw Error(d(61));if(n=a.__html,n!=null){if(i.children!=null)throw Error(d(60));(l!=null?l.__html:void 0)!==n&&(e.innerHTML=n)}}break;case"children":if(typeof a=="string")Ua(e,a);else if(typeof a=="number"||typeof a=="bigint")Ua(e,""+a);else return;break;case"onScroll":a!=null&&_e("scroll",e);return;case"onScrollEnd":a!=null&&_e("scrollend",e);return;case"onClick":a!=null&&(e.onclick=an);return;case"suppressContentEditableWarning":case"suppressHydrationWarning":case"innerHTML":case"ref":return;case"innerText":case"textContent":return;default:if(!Tu.hasOwnProperty(n))e:{if(n[0]==="o"&&n[1]==="n"&&(i=n.endsWith("Capture"),l=n.slice(2,i?n.length-7:void 0),t=e[Nt]||null,t=t!=null?t[n]:null,typeof t=="function"&&e.removeEventListener(l,t,i),typeof a=="function")){typeof t!="function"&&t!==null&&(n in e?e[n]=null:e.hasAttribute(n)&&e.removeAttribute(n)),e.addEventListener(l,a,i);break e}Se=!0,n in e?e[n]=a:a===!0?e.setAttribute(n,""):Rl(e,n,a)}return}Se=!0}function _t(e,t,n){switch(t){case"div":case"span":case"svg":case"path":case"a":case"g":case"p":case"li":break;case"img":_e("error",e),_e("load",e);var a=!1,i=!1,l;for(l in n)if(n.hasOwnProperty(l)){var s=n[l];if(s!=null)switch(l){case"src":a=!0;break;case"srcSet":i=!0;break;case"children":case"dangerouslySetInnerHTML":throw Error(d(137,t));default:qe(e,t,l,s,n,null)}}i&&qe(e,t,"srcSet",n.srcSet,n,null),a&&qe(e,t,"src",n.src,n,null);return;case"input":_e("invalid",e);var u=l=s=i=null,_=null,h=null;for(a in n)if(n.hasOwnProperty(a)){var S=n[a];if(S!=null)switch(a){case"name":i=S;break;case"type":s=S;break;case"checked":_=S;break;case"defaultChecked":h=S;break;case"value":l=S;break;case"defaultValue":u=S;break;case"children":case"dangerouslySetInnerHTML":if(S!=null)throw Error(d(137,t));break;default:qe(e,t,a,S,n,null)}}Du(e,l,u,_,h,s,i,!1);return;case"select":_e("invalid",e),a=s=l=null;for(i in n)if(n.hasOwnProperty(i)&&(u=n[i],u!=null))switch(i){case"value":l=u;break;case"defaultValue":s=u;break;case"multiple":a=u;default:qe(e,t,i,u,n,null)}t=l,n=s,e.multiple=!!a,t!=null?za(e,!!a,t,!1):n!=null&&za(e,!!a,n,!0);return;case"textarea":_e("invalid",e),l=i=a=null;for(s in n)if(n.hasOwnProperty(s)&&(u=n[s],u!=null))switch(s){case"value":a=u;break;case"defaultValue":i=u;break;case"children":l=u;break;case"dangerouslySetInnerHTML":if(u!=null)throw Error(d(91));break;default:qe(e,t,s,u,n,null)}zu(e,a,i,l);return;case"option":for(_ in n)if(n.hasOwnProperty(_)&&(a=n[_],a!=null))switch(_){case"selected":e.selected=a&&typeof a!="function"&&typeof a!="symbol";break;default:qe(e,t,_,a,n,null)}return;case"dialog":_e("beforetoggle",e),_e("toggle",e),_e("cancel",e),_e("close",e);break;case"iframe":case"object":_e("load",e);break;case"video":case"audio":for(a=0;a<cl.length;a++)_e(cl[a],e);break;case"image":_e("error",e),_e("load",e);break;case"details":_e("toggle",e);break;case"embed":case"source":case"link":_e("error",e),_e("load",e);case"area":case"base":case"br":case"col":case"hr":case"keygen":case"meta":case"param":case"track":case"wbr":case"menuitem":for(h in n)if(n.hasOwnProperty(h)&&(a=n[h],a!=null))switch(h){case"children":case"dangerouslySetInnerHTML":throw Error(d(137,t));default:qe(e,t,h,a,n,null)}return;default:if(gs(t)){for(S in n)n.hasOwnProperty(S)&&(a=n[S],a!==void 0&&hc(e,t,S,a,n,void 0));return}}for(u in n)n.hasOwnProperty(u)&&(a=n[u],a!=null&&qe(e,t,u,a,n,null))}var hy={};function gy(e,t,n,a){switch(t){case"div":case"span":case"svg":case"path":case"a":case"g":case"p":case"li":break;case"input":var i=null,l=null,s=null,u=null,_=null,h=null,S=null;for(w in n){var T=n[w];if(n.hasOwnProperty(w)&&T!=null)switch(w){case"checked":break;case"value":break;case"defaultValue":_=T;default:a.hasOwnProperty(w)||qe(e,t,w,null,a,T)}}for(var v in a){var w=a[v];if(T=n[v],a.hasOwnProperty(v)&&(w!=null||T!=null))switch(v){case"type":w!==T&&(Se=!0),l=w;break;case"name":w!==T&&(Se=!0),i=w;break;case"checked":w!==T&&(Se=!0),h=w;break;case"defaultChecked":w!==T&&(Se=!0),S=w;break;case"value":w!==T&&(Se=!0),s=w;break;case"defaultValue":w!==T&&(Se=!0),u=w;break;case"children":case"dangerouslySetInnerHTML":if(w!=null)throw Error(d(137,t));break;default:w!==T&&qe(e,t,v,w,a,T)}}ys(e,s,u,_,h,S,l,i);return;case"select":w=s=u=v=null;for(l in n)if(_=n[l],n.hasOwnProperty(l)&&_!=null)switch(l){case"value":break;case"multiple":w=_;default:a.hasOwnProperty(l)||qe(e,t,l,null,a,_)}for(i in a)if(l=a[i],_=n[i],a.hasOwnProperty(i)&&(l!=null||_!=null))switch(i){case"value":l!==_&&(Se=!0),v=l;break;case"defaultValue":l!==_&&(Se=!0),u=l;break;case"multiple":l!==_&&(Se=!0),s=l;default:l!==_&&qe(e,t,i,l,a,_)}t=u,n=s,a=w,v!=null?za(e,!!n,v,!1):!!a!=!!n&&(t!=null?za(e,!!n,t,!0):za(e,!!n,n?[]:"",!1));return;case"textarea":w=v=null;for(u in n)if(i=n[u],n.hasOwnProperty(u)&&i!=null&&!a.hasOwnProperty(u))switch(u){case"value":break;case"children":break;default:qe(e,t,u,null,a,i)}for(s in a)if(i=a[s],l=n[s],a.hasOwnProperty(s)&&(i!=null||l!=null))switch(s){case"value":i!==l&&(Se=!0),v=i;break;case"defaultValue":i!==l&&(Se=!0),w=i;break;case"children":break;case"dangerouslySetInnerHTML":if(i!=null)throw Error(d(91));break;default:i!==l&&qe(e,t,s,i,a,l)}Mu(e,v,w);return;case"option":for(var G in n)if(v=n[G],n.hasOwnProperty(G)&&v!=null&&!a.hasOwnProperty(G))switch(G){case"selected":e.selected=!1;break;default:qe(e,t,G,null,a,v)}for(_ in a)if(v=a[_],w=n[_],a.hasOwnProperty(_)&&v!==w&&(v!=null||w!=null))switch(_){case"selected":v!==w&&(Se=!0),e.selected=v&&typeof v!="function"&&typeof v!="symbol";break;default:qe(e,t,_,v,a,w)}return;case"img":case"link":case"area":case"base":case"br":case"col":case"embed":case"hr":case"keygen":case"meta":case"param":case"source":case"track":case"wbr":case"menuitem":for(var K in n)v=n[K],n.hasOwnProperty(K)&&v!=null&&!a.hasOwnProperty(K)&&qe(e,t,K,null,a,v);for(h in a)if(v=a[h],w=n[h],a.hasOwnProperty(h)&&v!==w&&(v!=null||w!=null))switch(h){case"children":case"dangerouslySetInnerHTML":if(v!=null)throw Error(d(137,t));break;default:qe(e,t,h,v,a,w)}return;default:if(gs(t)){for(var oe in n)v=n[oe],n.hasOwnProperty(oe)&&v!==void 0&&!a.hasOwnProperty(oe)&&hc(e,t,oe,void 0,a,v);for(S in a)v=a[S],w=n[S],!a.hasOwnProperty(S)||v===w||v===void 0&&w===void 0||hc(e,t,S,v,a,w);return}}for(var y in n)v=n[y],n.hasOwnProperty(y)&&v!=null&&!a.hasOwnProperty(y)&&qe(e,t,y,null,a,v);for(T in a)v=a[T],w=n[T],!a.hasOwnProperty(T)||v===w||v==null&&w==null||qe(e,t,T,v,a,w)}function Ym(e){switch(e){case"css":case"script":case"font":case"img":case"image":case"input":case"link":return!0;default:return!1}}function by(){if(typeof performance.getEntriesByType=="function"){for(var e=0,t=0,n=performance.getEntriesByType("resource"),a=0;a<n.length;a++){var i=n[a],l=i.transferSize,s=i.initiatorType,u=i.duration;if(l&&u&&Ym(s)){for(s=0,u=i.responseEnd,a+=1;a<n.length;a++){var _=n[a],h=_.startTime;if(h>u)break;var S=_.transferSize,T=_.initiatorType;S&&Ym(T)&&(_=_.responseEnd,s+=S*(_<u?1:(u-h)/(_-h)))}if(--a,t+=8*(l+s)/(i.duration/1e3),e++,10<e)break}}if(0<e)return t/e/1e6}return navigator.connection&&(e=navigator.connection.downlink,typeof e=="number")?e:5}var gc=null,bc=null;function dl(e){return e.nodeType===9?e:e.ownerDocument}function Qm(e){switch(e){case"http://www.w3.org/2000/svg":return 1;case"http://www.w3.org/1998/Math/MathML":return 2;default:return 0}}function Pm(e,t){if(e===0)switch(t){case"svg":return 1;case"math":return 2;default:return 0}return e===1&&t==="foreignObject"?0:e}function Km(e,t,n,a){return n=dl(n).createElement(e),n[st]=a,n[Nt]=t,_t(n,e,t),et(n),n}function xc(e,t){return e==="textarea"||e==="noscript"||typeof t.children=="string"||typeof t.children=="number"||typeof t.children=="bigint"||typeof t.dangerouslySetInnerHTML=="object"&&t.dangerouslySetInnerHTML!==null&&t.dangerouslySetInnerHTML.__html!=null}var wc=null;function xy(){var e=window.event;return e&&e.type==="popstate"?e===wc?!1:(wc=e,!0):(wc=null,!1)}var Nc=typeof setTimeout=="function"?setTimeout:void 0,wy=typeof clearTimeout=="function"?clearTimeout:void 0,Xm=typeof Promise=="function"?Promise:void 0,Fm=typeof requestAnimationFrame=="function"?requestAnimationFrame:Nc,Ny=typeof queueMicrotask=="function"?queueMicrotask:typeof Xm<"u"?function(e){return Xm.resolve(null).then(e).catch(Sy)}:Nc;function Sy(e){setTimeout(function(){throw e})}function ea(e){return e==="head"}function Zm(e,t){var n=t,a=0;do{var i=n.nextSibling;if(e.removeChild(n),i&&i.nodeType===8)if(n=i.data,n==="/$"||n==="/&"){if(a===0){e.removeChild(i),bi(t);return}a--}else if(n==="$"||n==="$?"||n==="$~"||n==="$!"||n==="&")a++;else if(n==="html")Oc(e.ownerDocument.documentElement);else if(n==="head"){n=e.ownerDocument.head,Oc(n);for(var l=n.firstChild;l;){var s=l.nextSibling,u=l.nodeName;l[Ci]||u==="SCRIPT"||u==="STYLE"||u==="LINK"&&l.rel.toLowerCase()==="stylesheet"||n.removeChild(l),l=s}}else n==="body"&&Oc(e.ownerDocument.body);n=i}while(n);bi(t)}function Jm(e,t){var n=e;e=0;do{var a=n.nextSibling;if(n.nodeType===1?t?(n._stashedDisplay=n.style.display,n.style.display="none"):(n.style.display=n._stashedDisplay||"",n.getAttribute("style")===""&&n.removeAttribute("style")):n.nodeType===3&&(t?(n._stashedText=n.nodeValue,n.nodeValue=""):n.nodeValue=n._stashedText||""),a&&a.nodeType===8)if(n=a.data,n==="/$"){if(e===0)break;e--}else n!=="$"&&n!=="$?"&&n!=="$~"&&n!=="$!"||e++;n=a}while(n)}function $m(e,t,n){if(t=CSS.escape(t)!==t?"r-"+btoa(t).replace(/=/g,""):t,e.style.viewTransitionName=t,n!=null&&(e.style.viewTransitionClass=n),n=getComputedStyle(e),n.display==="inline"){if(t=e.getClientRects(),t.length===1)var a=1;else for(var i=a=0;i<t.length;i++){var l=t[i];0<l.width&&0<l.height&&a++}a===1&&(e=e.style,e.display=t.length===1?"inline-block":"block",e.marginTop="-"+n.paddingTop,e.marginBottom="-"+n.paddingBottom)}}function Wm(e,t){e=e.style,t=t.style;var n=t!=null?t.hasOwnProperty("viewTransitionName")?t.viewTransitionName:t.hasOwnProperty("view-transition-name")?t["view-transition-name"]:null:null;e.viewTransitionName=n==null||typeof n=="boolean"?"":(""+n).trim(),n=t!=null?t.hasOwnProperty("viewTransitionClass")?t.viewTransitionClass:t.hasOwnProperty("view-transition-class")?t["view-transition-class"]:null:null,e.viewTransitionClass=n==null||typeof n=="boolean"?"":(""+n).trim(),e.display==="inline-block"&&(t==null?e.display=e.margin="":(n=t.display,e.display=n==null||typeof n=="boolean"?"":n,n=t.margin,n!=null?e.margin=n:(n=t.hasOwnProperty("marginTop")?t.marginTop:t["margin-top"],e.marginTop=n==null||typeof n=="boolean"?"":n,t=t.hasOwnProperty("marginBottom")?t.marginBottom:t["margin-bottom"],e.marginBottom=t==null||typeof t=="boolean"?"":t)))}function jy(e,t,n){return n=n.ownerDocument.defaultView,{rect:e,abs:t.position==="absolute"||t.position==="fixed",clip:t.clipPath!=="none"||t.overflow!=="visible"||t.filter!=="none"||t.mask!=="none"||t.mask!=="none"||t.borderRadius!=="0px",view:0<=e.bottom&&0<=e.right&&e.top<=n.innerHeight&&e.left<=n.innerWidth}}function Sc(e){var t=e.getBoundingClientRect(),n=getComputedStyle(e);return jy(t,n,e)}function Ey(e){return e.documentElement.clientHeight}function ky(e){this.addEventListener("load",e),this.addEventListener("error",e)}function Ty(e,t,n,a,i,l,s,u,_){var h=t.nodeType===9?t:t.ownerDocument;try{var S=h.startViewTransition({update:function(){var v=h.defaultView,w=v.navigation&&v.navigation.transition,G=h.fonts.status;a();var K=[];if(G==="loaded"&&(Ey(h),h.fonts.status==="loading"&&K.push(h.fonts.ready)),G=K.length,e!==null)for(var oe=e.suspenseyImages,y=0,f=0;f<oe.length;f++){var b=oe[f];if(!b.complete){var k=b.getBoundingClientRect();if(0<k.bottom&&0<k.right&&k.top<v.innerHeight&&k.left<v.innerWidth){if(y+=xf(b),y>Yr){K.length=G;break}b=new Promise(ky.bind(b)),K.push(b)}}}if(0<K.length)return v=Promise.race([Promise.all(K),new Promise(function(Y){return setTimeout(Y,500)})]).then(i,i),(w?Promise.allSettled([w.finished,v]):v).then(l,l);if(i(),w)return w.finished.then(l,l);l()},types:n});h.__reactViewTransition=S;var T=[];return S.ready.then(function(){for(var v=h.documentElement.getAnimations({subtree:!0}),w=0;w<v.length;w++){var G=v[w],K=G.effect,oe=K.pseudoElement;if(oe!=null&&oe.startsWith("::view-transition")){T.push(G),G=K.getKeyframes();for(var y=oe=void 0,f=!0,b=0;b<G.length;b++){var k=G[b],Y=k.width;if(oe===void 0)oe=Y;else if(oe!==Y){f=!1;break}if(Y=k.height,y===void 0)y=Y;else if(y!==Y){f=!1;break}delete k.width,delete k.height,k.transform==="none"&&delete k.transform}f&&oe!==void 0&&y!==void 0&&(K.setKeyframes(G),f=getComputedStyle(K.target,K.pseudoElement),f.width!==oe||f.height!==y)&&(f=G[0],f.width=oe,f.height=y,f=G[G.length-1],f.width=oe,f.height=y,K.setKeyframes(G))}}s()},function(v){h.__reactViewTransition===S&&(h.__reactViewTransition=null);try{if(typeof v=="object"&&v!==null)switch(v.name){case"InvalidStateError":(v.message==="View transition was skipped because document visibility state is hidden."||v.message==="Skipping view transition because document visibility state has become hidden."||v.message==="Skipping view transition because viewport size changed."||v.message==="Transition was aborted because of invalid state")&&(v=null)}v!==null&&_(v)}finally{a(),i(),s()}}),S.finished.finally(function(){for(var v=0;v<T.length;v++)T[v].cancel();h.__reactViewTransition===S&&(h.__reactViewTransition=null),u()}),S}catch{return a(),i(),s(),null}}function Ca(e,t){this._scope=document.documentElement,this._selector="::view-transition-"+e+"("+t+")"}Ca.prototype.animate=function(e,t){return t=typeof t=="number"?{duration:t}:z({},t),t.pseudoElement=this._selector,this._scope.animate(e,t)},Ca.prototype.getAnimations=function(){for(var e=this._scope,t=this._selector,n=e.getAnimations({subtree:!0}),a=[],i=0;i<n.length;i++){var l=n[i].effect;l!==null&&l.target===e&&l.pseudoElement===t&&a.push(n[i])}return a},Ca.prototype.getComputedStyle=function(){return getComputedStyle(this._scope,this._selector)};function ef(e){return{name:e,group:new Ca("group",e),imagePair:new Ca("image-pair",e),old:new Ca("old",e),new:new Ca("new",e)}}function Vt(e){this._fragmentFiber=e,this._observers=this._eventListeners=null}Vt.prototype.addEventListener=function(e,t,n){var a=null,i=null;if(!(n!=null&&typeof n!="boolean"&&(a=n.signal||null,a!==null&&a.aborted))){this._eventListeners===null&&(this._eventListeners=[]);var l=this._eventListeners;if(nf(l,e,t,n)===-1){var s=this,u=t;n!=null&&typeof n!="boolean"&&n.once===!0&&(u=function(_){s.removeEventListener(e,t,n),typeof t=="function"?t.call(this,_):t.handleEvent(_)}),a!==null&&(i=s.removeEventListener.bind(s,e,t,n),a.addEventListener("abort",i,{once:!0}),i=a.removeEventListener.bind(a,"abort",i)),a=fi(n),l.push({type:e,listener:t,optionsOrUseCapture:n,attachedListener:u,cleanup:i}),g(this._fragmentFiber.child,!1,Cy,e,u,a)}this._eventListeners=l}};function Cy(e,t,n,a){return I(e).addEventListener(t,n,a),!1}Vt.prototype.removeEventListener=function(e,t,n){var a=this._eventListeners;if(a!==null&&(t=nf(a,e,t,n),t!==-1)){var i=a[t];n=i.attachedListener;var l=i.cleanup;i=fi(i.optionsOrUseCapture),g(this._fragmentFiber.child,!1,Ay,e,n,i),a.splice(t,1),l!==null&&l()}};function Ay(e,t,n,a){return I(e).removeEventListener(t,n,a),!1}function fi(e){return e!=null&&typeof e!="boolean"&&(e.once===!0||e.signal instanceof AbortSignal)?{capture:e.capture,passive:e.passive}:e}function tf(e){return e==null?"c=0":typeof e=="boolean"?"c="+(e?"1":"0"):"c="+(e.capture?"1":"0")}function nf(e,t,n,a){if(e.length===0)return-1;a=tf(a);for(var i=0;i<e.length;i++){var l=e[i];if(l.type===t&&l.listener===n&&tf(l.optionsOrUseCapture)===a)return i}return-1}Vt.prototype.dispatchEvent=function(e){var t=q(this._fragmentFiber);if(t===null)return!0;t=I(t);var n=this._eventListeners;if(n!==null&&0<n.length||!e.bubbles){var a=t.nodeType===9?t.createComment(""):document.createTextNode("");if(n)for(var i=0;i<n.length;i++){var l=n[i];a.addEventListener(l.type,l.attachedListener,fi(l.optionsOrUseCapture))}if(t.appendChild(a),e=a.dispatchEvent(e),n)for(i=0;i<n.length;i++)l=n[i],a.removeEventListener(l.type,l.attachedListener,fi(l.optionsOrUseCapture));return t.removeChild(a),e}return t.dispatchEvent(e)},Vt.prototype.focus=function(e){g(this._fragmentFiber.child,!0,af,e,void 0,void 0)};function af(e,t){return e.tag===6?!1:(e=I(e),Gy(e,t))}Vt.prototype.focusLast=function(e){var t=[];g(this._fragmentFiber.child,!0,jc,t,void 0,void 0);for(var n=t.length-1;0<=n&&!af(t[n],e);n--);};function jc(e,t){return t.push(e),!1}Vt.prototype.blur=function(){var e=q(this._fragmentFiber);e!==null&&(e=I(e),e=dl(e).activeElement,e!==null&&g(this._fragmentFiber.child,!1,Oy,e,void 0,void 0))};function Oy(e,t){return e.tag===6?!1:(e=I(e),e===t||e.contains(t)?(t.blur(),!0):!1)}Vt.prototype.observeUsing=function(e){this._observers===null&&(this._observers=new Set),this._observers.add(e),g(this._fragmentFiber.child,!1,qy,e,void 0,void 0)};function qy(e,t){return e.tag===6||(e=I(e),t.observe(e)),!1}Vt.prototype.unobserveUsing=function(e){var t=this._observers;if(t!==null&&t.has(e)){t.delete(e),g(this._fragmentFiber.child,!1,Ry,e,void 0,void 0);for(var n=t=0;n<tn.length;n++){var a=tn[n];a.fragmentInstance===this&&a.observer===e?e.unobserve(a.instance):tn[t++]=a}tn.length=t}};function Ry(e,t){return e.tag===6||(e=I(e),t.unobserve(e)),!1}var tn=[],Ec=!1;function Dy(e,t,n){tn.push({fragmentInstance:e,observer:t,instance:n}),Ec||(Ec=!0,Iy(function(){Ec=!1;var a=tn;tn=[];for(var i=0;i<a.length;i++){var l=a[i];l.observer.unobserve(l.instance)}}))}Vt.prototype.getClientRects=function(){var e=[];return g(this._fragmentFiber.child,!1,My,e,void 0,void 0),e};function My(e,t){if(e.tag===6){e=e.stateNode;var n=e.ownerDocument.createRange();n.selectNodeContents(e),t.push.apply(t,n.getClientRects())}else e=I(e),t.push.apply(t,e.getClientRects());return!1}Vt.prototype.getRootNode=function(e){var t=q(this._fragmentFiber);return t===null?this:I(t).getRootNode(e)},Vt.prototype.compareDocumentPosition=function(e){var t=q(this._fragmentFiber);if(t===null)return Node.DOCUMENT_POSITION_DISCONNECTED;var n=[];g(this._fragmentFiber.child,!1,jc,n,void 0,void 0);var a=I(t);if(n.length===0){if(n=a,$(this._fragmentFiber)){e:{for(t=this._fragmentFiber.return;t!==null;){if(t.tag===4){t=t.stateNode.containerInfo;break e}if(t.tag===3||t.tag===5||t.tag===27)break;t=t.return}t=null}t!=null&&(n=t)}t=this._fragmentFiber;var i=a=n.compareDocumentPosition(e);return n===e?i=Node.DOCUMENT_POSITION_CONTAINS:a&Node.DOCUMENT_POSITION_CONTAINED_BY&&(n=M(t)[1],n===null?i=Node.DOCUMENT_POSITION_PRECEDING:(e=I(n).compareDocumentPosition(e),i=e===0||e&Node.DOCUMENT_POSITION_FOLLOWING?Node.DOCUMENT_POSITION_FOLLOWING:Node.DOCUMENT_POSITION_PRECEDING)),i|=Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC}t=I(n[0]),i=I(n[n.length-1]);var l=$(this._fragmentFiber)?t.parentElement:a;if(l==null)return Node.DOCUMENT_POSITION_DISCONNECTED;a=l.compareDocumentPosition(t)&Node.DOCUMENT_POSITION_CONTAINED_BY,l=l.compareDocumentPosition(i)&Node.DOCUMENT_POSITION_CONTAINED_BY;var s=t.compareDocumentPosition(e),u=i.compareDocumentPosition(e),_=s&Node.DOCUMENT_POSITION_CONTAINED_BY||u&Node.DOCUMENT_POSITION_CONTAINED_BY;return u=a&&l&&s&Node.DOCUMENT_POSITION_FOLLOWING&&u&Node.DOCUMENT_POSITION_PRECEDING,t=a&&t===e||l&&i===e||_||u?Node.DOCUMENT_POSITION_CONTAINED_BY:!a&&t===e||!l&&i===e?Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC:s,t&Node.DOCUMENT_POSITION_DISCONNECTED||t&Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC||zy(t,this._fragmentFiber,n[0],n[n.length-1],e)?t:Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC};function zy(e,t,n,a,i){var l=oa(i);if(e&Node.DOCUMENT_POSITION_CONTAINED_BY){if(n=!!l)e:{for(;l!==null;){if(l.tag===7&&(l===t||l.alternate===t)){n=!0;break e}l=l.return}n=!1}return n}if(e&Node.DOCUMENT_POSITION_CONTAINS){if(l===null)return l=i.ownerDocument,i===l||i===l.documentElement||i===l.body;e:{for(l=t,t=q(t);l!==null;){if(!(l.tag!==5&&l.tag!==3&&l.tag!==27||l!==t&&l.alternate!==t)){l=!0;break e}l=l.return}l=!1}return l}return e&Node.DOCUMENT_POSITION_PRECEDING?((t=!!l)&&!(t=l===n)&&(t=C(n,l,xe),t===null?t=!1:(g(t,!0,F,l,n),l=J,J=null,t=l!==null)),t):e&Node.DOCUMENT_POSITION_FOLLOWING?((t=!!l)&&!(t=l===a)&&(t=C(a,l,xe),t===null?t=!1:(g(t,!0,re,l,a),l=J,ae=J=null,t=l!==null)),t):!1}function lf(e,t){var n=e.ownerDocument.createRange();n.selectNodeContents(e),e=n.getBoundingClientRect(),window.scrollTo(window.scrollX+e.left,t?window.scrollY+e.top:window.scrollY+e.bottom-window.innerHeight)}Vt.prototype.scrollIntoView=function(e){if(typeof e=="object")throw Error(d(566));var t=[];g(this._fragmentFiber.child,!1,jc,t,void 0,void 0);var n=e!==!1;if(t.length===0){var a=M(this._fragmentFiber);if(a=n?a[1]||a[0]||q(this._fragmentFiber):a[0]||a[1],a===null)return;if(a.tag===6){e=I(a),lf(e,n);return}if(a=I(a),a.nodeType!==9){if(a.nodeType===11){n="host"in a?a.host:null,n!==null&&n.scrollIntoView(e);return}a.scrollIntoView(e)}}for(a=n?t.length-1:0;a!==(n?-1:t.length);){var i=t[a];i.tag===6?(i=I(i),lf(i,n)):I(i).scrollIntoView(e),a+=n?-1:1}};function Uy(e,t){return e=I(e),rf(e,t),!1}function rf(e,t){e.reactFragments==null&&(e.reactFragments=new Set),e.reactFragments.add(t)}function sf(e,t){var n=t._eventListeners;if(n!==null)for(var a=0;a<n.length;a++){var i=n[a];e.addEventListener(i.type,i.attachedListener,fi(i.optionsOrUseCapture))}e.nodeType!==3&&(n=t._observers,n!==null&&n.forEach(function(l){for(var s=0,u=0;u<tn.length;u++){var _=tn[u];(_.fragmentInstance!==t||_.observer!==l||_.instance!==e)&&(tn[s++]=_)}tn.length=s,l.observe(e)}),rf(e,t))}function By(e,t){var n=t._eventListeners;if(n!==null)for(var a=0;a<n.length;a++){var i=n[a];e.removeEventListener(i.type,i.attachedListener,fi(i.optionsOrUseCapture))}e.nodeType!==3&&(n=t._observers,n!==null&&n.forEach(function(l){typeof l.rootMargin=="string"?Dy(t,l,e):l.unobserve(e)}),e.reactFragments!=null&&e.reactFragments.delete(t))}function kc(e){var t=e.firstChild;for(t&&t.nodeType===10&&(t=t.nextSibling);t;){var n=t;switch(t=t.nextSibling,n.nodeName){case"HTML":case"HEAD":case"BODY":kc(n),ql(n);continue;case"SCRIPT":case"STYLE":continue;case"LINK":if(n.rel.toLowerCase()==="stylesheet")continue}e.removeChild(n)}}function Vy(e,t,n,a){for(;e.nodeType===1;){var i=n;if(e.nodeName.toLowerCase()!==t.toLowerCase()){if(!a&&(e.nodeName!=="INPUT"||e.type!=="hidden"))break}else if(a){if(!e[Ci])switch(t){case"meta":if(!e.hasAttribute("itemprop"))break;return e;case"link":if(l=e.getAttribute("rel"),l==="stylesheet"&&e.hasAttribute("data-precedence"))break;if(l!==i.rel||e.getAttribute("href")!==(i.href==null||i.href===""?null:i.href)||e.getAttribute("crossorigin")!==(i.crossOrigin==null?null:i.crossOrigin)||e.getAttribute("title")!==(i.title==null?null:i.title))break;return e;case"style":if(e.hasAttribute("data-precedence"))break;return e;case"script":if(l=e.getAttribute("src"),(l!==(i.src==null?null:i.src)||e.getAttribute("type")!==(i.type==null?null:i.type)||e.getAttribute("crossorigin")!==(i.crossOrigin==null?null:i.crossOrigin))&&l&&e.hasAttribute("async")&&!e.hasAttribute("itemprop"))break;return e;default:return e}}else if(t==="input"&&e.type==="hidden"){var l=i.name==null?null:""+i.name;if(i.type==="hidden"&&e.getAttribute("name")===l)return e}else return e;if(e=Kt(e.nextSibling),e===null)break}return null}function Ly(e,t,n){if(t==="")return null;for(;e.nodeType!==3;)if((e.nodeType!==1||e.nodeName!=="INPUT"||e.type!=="hidden")&&!n||(e=Kt(e.nextSibling),e===null))return null;return e}function of(e,t){for(;e.nodeType!==8;)if((e.nodeType!==1||e.nodeName!=="INPUT"||e.type!=="hidden")&&!t||(e=Kt(e.nextSibling),e===null))return null;return e}function Tc(e){return e.data==="$?"||e.data==="$~"}function Cc(e){return e.data==="$!"||e.data==="$?"&&e.ownerDocument.readyState!=="loading"}function Hy(e,t){var n=e.ownerDocument;if(e.data==="$~")e._reactRetry=t;else if(e.data!=="$?"||n.readyState!=="loading")t();else{var a=function(){t(),n.removeEventListener("DOMContentLoaded",a)};n.addEventListener("DOMContentLoaded",a),e._reactRetry=a}}function Kt(e){for(;e!=null;e=e.nextSibling){var t=e.nodeType;if(t===1||t===3)break;if(t===8){if(t=e.data,t==="$"||t==="$!"||t==="$?"||t==="$~"||t==="&"||t==="F!"||t==="F")break;if(t==="/$"||t==="/&")return null}}return e}var Ac=null;function cf(e){e=e.nextSibling;for(var t=0;e;){if(e.nodeType===8){var n=e.data;if(n==="/$"||n==="/&"){if(t===0)return Kt(e.nextSibling);t--}else n!=="$"&&n!=="$!"&&n!=="$?"&&n!=="$~"&&n!=="&"||t++}e=e.nextSibling}return null}function uf(e){e=e.previousSibling;for(var t=0;e;){if(e.nodeType===8){var n=e.data;if(n==="$"||n==="$!"||n==="$?"||n==="$~"||n==="&"){if(t===0)return e;t--}else n!=="/$"&&n!=="/&"||t++}e=e.previousSibling}return null}function Gy(e,t){function n(){a=!0}if(e.ownerDocument.activeElement===e)return!0;var a=!1;try{e.ownerDocument.addEventListener("focus",n,!0),(e.focus||HTMLElement.prototype.focus).call(e,t)}finally{e.ownerDocument.removeEventListener("focus",n,!0)}return a}function Iy(e){Fm(function(){Fm(function(t){return e(t)})})}function df(e,t,n){switch(t=dl(n),e){case"html":if(e=t.documentElement,!e)throw Error(d(452));return e;case"head":if(e=t.head,!e)throw Error(d(453));return e;case"body":if(e=t.body,!e)throw Error(d(454));return e;default:throw Error(d(451))}}function _f(e,t,n){for(var a in n){var i=n[a];n.hasOwnProperty(a)&&i!=null&&qe(e,t,a,null,hy,i)}n.dangerouslySetInnerHTML!=null&&(e.textContent=""),e.onclick===an&&(e.onclick=null),ql(e)}function Oc(e){for(var t=e.attributes;t.length;)e.removeAttributeNode(t[0]);ql(e)}var Xt=new Map,mf=new Set;function _l(e){if(typeof e.getRootNode=="function"){var t=e.getRootNode();if(t.nodeType===9||t.nodeType===11)return t}return e.nodeType===9?e:e.ownerDocument}var Cn=U.d;U.d={f:Yy,r:Qy,D:Py,C:Ky,L:Xy,m:Fy,X:Jy,S:Zy,M:$y};function Yy(){var e=Cn.f(),t=Mr();return e||t}function Qy(e){var t=Ra(e);t!==null&&t.tag===5&&t.type==="form"?f_(t):Cn.r(e)}var pi=typeof document>"u"?null:document;function ff(e,t,n){var a=pi;if(a&&typeof t=="string"&&t){var i=Lt(t);i='link[rel="'+e+'"][href="'+i+'"]',typeof n=="string"&&(i+='[crossorigin="'+n+'"]'),mf.has(i)||(mf.add(i),e={rel:e,crossOrigin:n,href:t},a.querySelector(i)===null&&(t=a.createElement("link"),_t(t,"link",e),et(t),a.head.appendChild(t)))}}function Py(e){Cn.D(e),ff("dns-prefetch",e,null)}function Ky(e,t){Cn.C(e,t),ff("preconnect",e,t)}function Xy(e,t,n){Cn.L(e,t,n);var a=pi;if(a&&e&&t){var i='link[rel="preload"][as="'+Lt(t)+'"]';t==="image"&&n&&n.imageSrcSet?(i+='[imagesrcset="'+Lt(n.imageSrcSet)+'"]',typeof n.imageSizes=="string"&&(i+='[imagesizes="'+Lt(n.imageSizes)+'"]')):i+='[href="'+Lt(e)+'"]';var l=i;switch(t){case"style":l=vi(e);break;case"script":l=yi(e)}if(!(Xt.has(l)||(e=z({rel:"preload",href:t==="image"&&n&&n.imageSrcSet?void 0:e,as:t},n),Xt.set(l,e),a.querySelector(i)!==null||t==="style"&&a.querySelector(ml(l))||t==="script"&&a.querySelector(fl(l))))){var s=a.createElement("link");_t(s,"link",e),t==="style"&&(s[Ol]=!0,s.onload=s.onerror=function(){Eu(s)}),et(s),a.head.appendChild(s)}}}function Fy(e,t){Cn.m(e,t);var n=pi;if(n&&e){var a=t&&typeof t.as=="string"?t.as:"script",i='link[rel="modulepreload"][as="'+Lt(a)+'"][href="'+Lt(e)+'"]',l=i;switch(a){case"audioworklet":case"paintworklet":case"serviceworker":case"sharedworker":case"worker":case"script":l=yi(e)}if(!Xt.has(l)&&(e=z({rel:"modulepreload",href:e},t),Xt.set(l,e),n.querySelector(i)===null)){switch(a){case"audioworklet":case"paintworklet":case"serviceworker":case"sharedworker":case"worker":case"script":if(n.querySelector(fl(l)))return}a=n.createElement("link"),_t(a,"link",e),et(a),n.head.appendChild(a)}}}function Zy(e,t,n){Cn.S(e,t,n);var a=pi;if(a&&e){var i=Da(a).hoistableStyles,l=vi(e);t=t||"default";var s=i.get(l);if(!s){var u={loading:0,preload:null};if(s=a.querySelector(ml(l)))u.loading=5;else{e=z({rel:"stylesheet",href:e,"data-precedence":t},n),(n=Xt.get(l))&&qc(e,n);var _=s=a.createElement("link");et(_),_t(_,"link",e),_._p=new Promise(function(h,S){_.onload=h,_.onerror=S}),_.addEventListener("load",function(){u.loading|=1}),_.addEventListener("error",function(){u.loading|=2}),u.loading|=4,Gr(s,t,a)}s={type:"stylesheet",instance:s,count:1,state:u},i.set(l,s)}}}function Jy(e,t){Cn.X(e,t);var n=pi;if(n&&e){var a=Da(n).hoistableScripts,i=yi(e),l=a.get(i);l||(l=n.querySelector(fl(i)),l||(e=z({src:e,async:!0},t),(t=Xt.get(i))&&Rc(e,t),l=n.createElement("script"),et(l),_t(l,"link",e),n.head.appendChild(l)),l={type:"script",instance:l,count:1,state:null},a.set(i,l))}}function $y(e,t){Cn.M(e,t);var n=pi;if(n&&e){var a=Da(n).hoistableScripts,i=yi(e),l=a.get(i);l||(l=n.querySelector(fl(i)),l||(e=z({src:e,async:!0,type:"module"},t),(t=Xt.get(i))&&Rc(e,t),l=n.createElement("script"),et(l),_t(l,"link",e),n.head.appendChild(l)),l={type:"script",instance:l,count:1,state:null},a.set(i,l))}}function pf(e,t,n,a){var i=(i=rt.current)?_l(i):null;if(!i)throw Error(d(446));switch(e){case"meta":case"title":return null;case"style":return typeof n.precedence=="string"&&typeof n.href=="string"?(n=vi(n.href),t=Da(i).hoistableStyles,a=t.get(n),a||(a={type:"style",instance:null,count:0,state:null},t.set(n,a)),a):{type:"void",instance:null,count:0,state:null};case"link":if(n.rel==="stylesheet"&&typeof n.href=="string"&&typeof n.precedence=="string"){e=vi(n.href);var l=Da(i).hoistableStyles,s=l.get(e);if(s||(i=i.ownerDocument||i,s={type:"stylesheet",instance:null,count:0,state:{loading:0,preload:null}},l.set(e,s),(l=i.querySelector(ml(e)))?l._p||(s.instance=l,s.state.loading=5):(l=Xt.get(e),l||(l={rel:"preload",as:"style",href:n.href,crossOrigin:n.crossOrigin,integrity:n.integrity,media:n.media,hrefLang:n.hrefLang,referrerPolicy:n.referrerPolicy},Xt.set(e,l)),Wy(i,e,l,s.state))),t&&a===null)throw Error(d(528,""));return s}if(t&&a!==null)throw Error(d(529,""));return null;case"script":return t=n.async,n=n.src,typeof n=="string"&&t&&typeof t!="function"&&typeof t!="symbol"?(n=yi(n),t=Da(i).hoistableScripts,a=t.get(n),a||(a={type:"script",instance:null,count:0,state:null},t.set(n,a)),a):{type:"void",instance:null,count:0,state:null};default:throw Error(d(444,e))}}function vi(e){return'href="'+Lt(e)+'"'}function ml(e){return'link[rel="stylesheet"]['+e+"]"}function vf(e){return z({},e,{"data-precedence":e.precedence,precedence:null})}function Wy(e,t,n,a){if(t=e.querySelector('link[rel="preload"][as="style"]['+t+"]")){if(t[Ol]!==!0){a.loading=1;return}}else t=e.createElement("link"),t[Ol]=!0,t.onload=t.onerror=Eu.bind(null,t),_t(t,"link",n),et(t),e.head.appendChild(t);a.preload=t,t.addEventListener("load",function(){return a.loading|=1}),t.addEventListener("error",function(){return a.loading|=2})}function yi(e){return'[src="'+Lt(e)+'"]'}function fl(e){return"script[async]"+e}function yf(e,t,n){if(t.count++,t.instance===null)switch(t.type){case"style":var a=e.querySelector('style[data-href~="'+Lt(n.href)+'"]');if(a)return t.instance=a,et(a),a;var i=z({},n,{"data-href":n.href,"data-precedence":n.precedence,href:null,precedence:null});return a=(e.ownerDocument||e).createElement("style"),et(a),_t(a,"style",i),Gr(a,n.precedence,e),t.instance=a;case"stylesheet":i=vi(n.href);var l=e.querySelector(ml(i));if(l)return t.state.loading|=4,t.instance=l,et(l),l;a=vf(n),(i=Xt.get(i))&&qc(a,i),l=(e.ownerDocument||e).createElement("link"),et(l);var s=l;return s._p=new Promise(function(u,_){s.onload=u,s.onerror=_}),_t(l,"link",a),t.state.loading|=4,Gr(l,n.precedence,e),t.instance=l;case"script":return l=yi(n.src),(i=e.querySelector(fl(l)))?(t.instance=i,et(i),i):(a=n,(i=Xt.get(l))&&(a=z({},n),Rc(a,i)),e=e.ownerDocument||e,i=e.createElement("script"),et(i),_t(i,"link",a),e.head.appendChild(i),t.instance=i);case"void":return null;default:throw Error(d(443,t.type))}else t.type==="stylesheet"&&(t.state.loading&4)===0&&(a=t.instance,t.state.loading|=4,Gr(a,n.precedence,e));return t.instance}function Gr(e,t,n){for(var a=n.querySelectorAll('link[rel="stylesheet"][data-precedence],style[data-precedence]'),i=a.length?a[a.length-1]:null,l=i,s=0;s<a.length;s++){var u=a[s];if(u.dataset.precedence===t)l=u;else if(l!==i)break}l?l.parentNode.insertBefore(e,l.nextSibling):(t=n.nodeType===9?n.head:n,t.insertBefore(e,t.firstChild))}function qc(e,t){e.crossOrigin==null&&(e.crossOrigin=t.crossOrigin),e.referrerPolicy==null&&(e.referrerPolicy=t.referrerPolicy),e.title==null&&(e.title=t.title)}function Rc(e,t){e.crossOrigin==null&&(e.crossOrigin=t.crossOrigin),e.referrerPolicy==null&&(e.referrerPolicy=t.referrerPolicy),e.integrity==null&&(e.integrity=t.integrity)}var Ir=null;function hf(e,t,n){if(Ir===null){var a=new Map,i=Ir=new Map;i.set(n,a)}else i=Ir,a=i.get(n),a||(a=new Map,i.set(n,a));if(a.has(e))return a;for(a.set(e,null),n=n.getElementsByTagName(e),i=0;i<n.length;i++){var l=n[i];if(!(l[Ci]||l[st]||e==="link"&&l.getAttribute("rel")==="stylesheet")&&l.namespaceURI!=="http://www.w3.org/2000/svg"){var s=l.getAttribute(t)||"";s=e+s;var u=a.get(s);u?u.push(l):a.set(s,[l])}}return a}function Dc(e,t,n){e=e.ownerDocument||e,e.head.insertBefore(n,t==="title"?e.querySelector("head > title"):null)}function eh(e,t,n){if(n===1||t.itemProp!=null)return!1;switch(e){case"meta":case"title":return!0;case"style":if(typeof t.precedence!="string"||typeof t.href!="string"||t.href==="")break;return!0;case"link":if(typeof t.rel!="string"||typeof t.href!="string"||t.href===""||t.onLoad||t.onError)break;switch(t.rel){case"stylesheet":return e=t.disabled,typeof t.precedence=="string"&&e==null;default:return!0}case"script":if(t.async&&typeof t.async!="function"&&typeof t.async!="symbol"&&!t.onLoad&&!t.onError&&t.src&&typeof t.src=="string")return!0}return!1}function gf(e,t){return e==="img"&&t.src!=null&&t.src!==""&&t.onLoad==null&&t.loading!=="lazy"}function bf(e){return!(e.type==="stylesheet"&&(e.state.loading&3)===0)}function xf(e){return(e.width||100)*(e.height||100)*(typeof devicePixelRatio=="number"?devicePixelRatio:1)*.25}function wf(e,t){typeof t.decode=="function"&&(e.imgCount++,t.complete||(e.imgBytes+=xf(t),e.suspenseyImages.push(t)),e=ah.bind(e),t.decode().then(e,e))}function th(e,t,n,a){if(n.type==="stylesheet"&&(typeof a.media!="string"||matchMedia(a.media).matches!==!1)&&(n.state.loading&4)===0){if(n.instance===null){var i=vi(a.href),l=t.querySelector(ml(i));if(l){t=l._p,t!==null&&typeof t=="object"&&typeof t.then=="function"&&(e.count++,e=pl.bind(e),t.then(e,e)),n.state.loading|=4,n.instance=l,et(l);return}l=t.ownerDocument||t,a=vf(a),(i=Xt.get(i))&&qc(a,i),l=l.createElement("link"),et(l);var s=l;s._p=new Promise(function(u,_){s.onload=u,s.onerror=_}),_t(l,"link",a),n.instance=l}e.stylesheets===null&&(e.stylesheets=new Map),e.stylesheets.set(n,t),(t=n.state.preload)&&(n.state.loading&3)===0&&(e.count++,n=pl.bind(e),t.addEventListener("load",n),t.addEventListener("error",n))}}var Yr=0;function nh(e,t){return e.stylesheets&&e.count===0&&Pr(e,e.stylesheets),0<e.count||0<e.imgCount?function(n){var a=setTimeout(function(){if(e.stylesheets&&Pr(e,e.stylesheets),e.unsuspend){var l=e.unsuspend;e.unsuspend=null,l()}},6e4+t);0<e.imgBytes&&Yr===0&&(Yr=62500*by());var i=setTimeout(function(){if(e.waitingForImages=!1,e.count===0&&(e.stylesheets&&Pr(e,e.stylesheets),e.unsuspend)){var l=e.unsuspend;e.unsuspend=null,l()}},(e.imgBytes>Yr?50:800)+t);return e.unsuspend=n,function(){e.unsuspend=null,clearTimeout(a),clearTimeout(i)}}:null}function Nf(e){if(e.count===0&&(e.imgCount===0||!e.waitingForImages)){if(e.stylesheets)Pr(e,e.stylesheets);else if(e.unsuspend){var t=e.unsuspend;e.unsuspend=null,t()}}}function pl(){this.count--,Nf(this)}function ah(){this.imgCount--,Nf(this)}var Qr=null;function Pr(e,t){e.stylesheets=null,e.unsuspend!==null&&(e.count++,Qr=new Map,t.forEach(ih,e),Qr=null,pl.call(e))}function ih(e,t){if(!(t.state.loading&4)){var n=Qr.get(e);if(n)var a=n.get(null);else{n=new Map,Qr.set(e,n);for(var i=e.querySelectorAll("link[data-precedence],style[data-precedence]"),l=0;l<i.length;l++){var s=i[l];(s.nodeName==="LINK"||s.getAttribute("media")!=="not all")&&(n.set(s.dataset.precedence,s),a=s)}a&&n.set(null,a)}i=t.instance,s=i.getAttribute("data-precedence"),l=n.get(s)||a,l===a&&n.set(null,i),n.set(s,i),this.count++,a=pl.bind(this),i.addEventListener("load",a),i.addEventListener("error",a),l?l.parentNode.insertBefore(i,l.nextSibling):(e=e.nodeType===9?e.head:e,e.insertBefore(i,e.firstChild)),t.state.loading|=4}}var hi={$$typeof:Le,Provider:null,Consumer:null,_currentValue:E,_currentValue2:E,_threadCount:0};function lh(e,t,n,a,i,l,s,u,_){this.tag=1,this.containerInfo=e,this.pingCache=this.current=this.pendingChildren=null,this.timeoutHandle=-1,this.callbackNode=this.next=this.pendingContext=this.context=this.cancelPendingCommit=null,this.callbackPriority=0,this.expirationTimes=ms(-1),this.entangledLanes=this.shellSuspendCounter=this.errorRecoveryDisabledLanes=this.expiredLanes=this.warmLanes=this.pingedLanes=this.suspendedLanes=this.pendingLanes=0,this.entanglements=ms(0),this.hiddenUpdates=ms(null),this.identifierPrefix=a,this.onUncaughtError=i,this.onCaughtError=l,this.onRecoverableError=s,this.pooledCache=null,this.pooledCacheLanes=0,this.formState=_,this.transitionTypes=null,this.incompleteTransitions=new Map}function Sf(e,t,n,a,i,l,s,u,_,h,S,T){return e=new lh(e,t,n,s,_,h,S,T,u),t=1,l===!0&&(t|=24),l=St(3,null,null,t),e.current=l,l.stateNode=e,t=Xs(),t.refCount++,e.pooledCache=t,t.refCount++,l.memoizedState={element:a,isDehydrated:n,cache:t},$s(l),e}function jf(e){return e?(e=Ya,e):Ya}function Ef(e,t,n,a,i,l){i=jf(i),a.context===null?a.context=i:a.pendingContext=i,a=Gn(t),a.payload={element:n},l=l===void 0?null:l,l!==null&&(a.callback=l),n=In(e,a,t),n!==null&&(Tt(n,e,t),Pi(n,e,t))}function kf(e,t){if(e=e.memoizedState,e!==null&&e.dehydrated!==null){var n=e.retryLane;e.retryLane=n!==0&&n<t?n:t}}function Mc(e,t){kf(e,t),(e=e.alternate)&&kf(e,t)}function Tf(e){if(e.tag===13||e.tag===31){var t=_a(e,67108864);t!==null&&Tt(t,e,67108864),Mc(e,67108864)}}function Cf(e){if(e.tag===13||e.tag===31){var t=Bt();t=fs(t);var n=_a(e,t);n!==null&&Tt(n,e,t),Mc(e,t)}}var gi=!0;function rh(e,t,n,a){var i=A.T;A.T=null;var l=U.p;try{U.p=2,zc(e,t,n,a)}finally{U.p=l,A.T=i}}function sh(e,t,n,a){var i=A.T;A.T=null;var l=U.p;try{U.p=8,zc(e,t,n,a)}finally{U.p=l,A.T=i}}function zc(e,t,n,a){if(gi){var i=Uc(a);if(i===null)yc(e,t,a,Kr,n),Of(e,a);else if(ch(i,e,t,n,a))a.stopPropagation();else if(Of(e,a),t&4&&-1<oh.indexOf(e)){for(;i!==null;){var l=Ra(i);if(l!==null)switch(l.tag){case 3:if(l=l.stateNode,l.current.memoizedState.isDehydrated){var s=sa(l.pendingLanes);if(s!==0){var u=l;for(u.pendingLanes|=2,u.entangledLanes|=2;s;){var _=1<<31-Ot(s);u.entanglements[1]|=_,s&=~_}fn(l),(ke&6)===0&&(qr=Ct()+500,ol(0))}}break;case 31:case 13:u=_a(l,2),u!==null&&Tt(u,l,2),Mr(),Mc(l,2)}if(l=Uc(a),l===null&&yc(e,t,a,Kr,n),l===i)break;i=l}i!==null&&a.stopPropagation()}else yc(e,t,a,null,n)}}function Uc(e){return e=xs(e),Bc(e)}var Kr=null;function Bc(e){if(Kr=null,e=oa(e),e!==null){var t=O(e);if(t===null)e=null;else{var n=t.tag;if(n===13){if(e=N(t),e!==null)return e;e=null}else if(n===31){if(e=L(t),e!==null)return e;e=null}else if(n===3){if(t.stateNode.current.memoizedState.isDehydrated)return t.tag===3?t.stateNode.containerInfo:null;e=null}else t!==e&&(e=null)}}return Kr=e,null}function Af(e){switch(e){case"beforetoggle":case"cancel":case"click":case"close":case"contextmenu":case"copy":case"cut":case"auxclick":case"dblclick":case"dragend":case"dragstart":case"drop":case"focusin":case"focusout":case"input":case"invalid":case"keydown":case"keypress":case"keyup":case"mousedown":case"mouseup":case"paste":case"pause":case"play":case"pointercancel":case"pointerdown":case"pointerup":case"ratechange":case"reset":case"seeked":case"submit":case"toggle":case"touchcancel":case"touchend":case"touchstart":case"volumechange":case"change":case"selectionchange":case"textInput":case"compositionstart":case"compositionend":case"compositionupdate":case"beforeblur":case"afterblur":case"beforeinput":case"blur":case"fullscreenchange":case"fullscreenerror":case"focus":case"hashchange":case"popstate":case"select":case"selectstart":return 2;case"drag":case"dragenter":case"dragexit":case"dragleave":case"dragover":case"mousemove":case"mouseout":case"mouseover":case"pointermove":case"pointerout":case"pointerover":case"resize":case"scroll":case"touchmove":case"wheel":case"mouseenter":case"mouseleave":case"pointerenter":case"pointerleave":return 8;case"message":switch(bp()){case fu:return 2;case pu:return 8;case El:case xp:return 32;case vu:return 268435456;default:return 32}default:return 32}}var Vc=!1,ta=null,na=null,aa=null,vl=new Map,yl=new Map,ia=[],oh="mousedown mouseup touchcancel touchend touchstart auxclick dblclick pointercancel pointerdown pointerup dragend dragstart drop compositionend compositionstart keydown keypress keyup input textInput copy cut paste click change contextmenu reset".split(" ");function Of(e,t){switch(e){case"focusin":case"focusout":ta=null;break;case"dragenter":case"dragleave":na=null;break;case"mouseover":case"mouseout":aa=null;break;case"pointerover":case"pointerout":vl.delete(t.pointerId);break;case"gotpointercapture":case"lostpointercapture":yl.delete(t.pointerId)}}function hl(e,t,n,a,i,l){return e===null||e.nativeEvent!==l?(e={blockedOn:t,domEventName:n,eventSystemFlags:a,nativeEvent:l,targetContainers:[i]},t!==null&&(t=Ra(t),t!==null&&Tf(t)),e):(e.eventSystemFlags|=a,t=e.targetContainers,i!==null&&t.indexOf(i)===-1&&t.push(i),e)}function ch(e,t,n,a,i){switch(t){case"focusin":return ta=hl(ta,e,t,n,a,i),!0;case"dragenter":return na=hl(na,e,t,n,a,i),!0;case"mouseover":return aa=hl(aa,e,t,n,a,i),!0;case"pointerover":var l=i.pointerId;return vl.set(l,hl(vl.get(l)||null,e,t,n,a,i)),!0;case"gotpointercapture":return l=i.pointerId,yl.set(l,hl(yl.get(l)||null,e,t,n,a,i)),!0}return!1}function qf(e){var t=oa(e.target);if(t!==null){var n=O(t);if(n!==null){if(t=n.tag,t===13){if(t=N(n),t!==null){e.blockedOn=t,Nu(e.priority,function(){Cf(n)});return}}else if(t===31){if(t=L(n),t!==null){e.blockedOn=t,Nu(e.priority,function(){Cf(n)});return}}else if(t===3&&n.stateNode.current.memoizedState.isDehydrated){e.blockedOn=n.tag===3?n.stateNode.containerInfo:null;return}}}e.blockedOn=null}function Xr(e){if(e.blockedOn!==null)return!1;for(var t=e.targetContainers;0<t.length;){var n=Uc(e.nativeEvent);if(n===null){n=e.nativeEvent;var a=new n.constructor(n.type,n);bs=a,n.target.dispatchEvent(a),bs=null}else return t=Ra(n),t!==null&&Tf(t),e.blockedOn=n,!1;t.shift()}return!0}function Rf(e,t,n){Xr(e)&&n.delete(t)}function uh(){Vc=!1,ta!==null&&Xr(ta)&&(ta=null),na!==null&&Xr(na)&&(na=null),aa!==null&&Xr(aa)&&(aa=null),vl.forEach(Rf),yl.forEach(Rf)}function Fr(e,t){e.blockedOn===t&&(e.blockedOn=null,Vc||(Vc=!0,o.unstable_scheduleCallback(o.unstable_NormalPriority,uh)))}var Zr=null;function Df(e){Zr!==e&&(Zr=e,o.unstable_scheduleCallback(o.unstable_NormalPriority,function(){Zr===e&&(Zr=null);for(var t=0;t<e.length;t+=3){var n=e[t],a=e[t+1],i=e[t+2];if(typeof a!="function"){if(Bc(a||n)===null)continue;break}var l=Ra(n);l!==null&&(e.splice(t,3),t-=3,bo(l,{pending:!0,data:i,method:n.method,action:a},a,i))}}))}function bi(e){function t(_){return Fr(_,e)}ta!==null&&Fr(ta,e),na!==null&&Fr(na,e),aa!==null&&Fr(aa,e),vl.forEach(t),yl.forEach(t);for(var n=0;n<ia.length;n++){var a=ia[n];a.blockedOn===e&&(a.blockedOn=null)}for(;0<ia.length&&(n=ia[0],n.blockedOn===null);)qf(n),n.blockedOn===null&&ia.shift();if(n=(e.ownerDocument||e).$$reactFormReplay,n!=null)for(a=0;a<n.length;a+=3){var i=n[a],l=n[a+1],s=i[Nt]||null;if(typeof l=="function")s||Df(n);else if(s){var u=null;if(l&&l.hasAttribute("formAction")){if(i=l,s=l[Nt]||null)u=s.formAction;else if(Bc(i)!==null)continue}else u=s.action;typeof u=="function"?n[a+1]=u:(n.splice(a,3),a-=3),Df(n)}}}function Mf(){function e(l){l.canIntercept&&l.info==="react-transition"&&l.intercept({handler:function(){return new Promise(function(s){return i=s})},focusReset:"manual",scroll:"manual"})}function t(){i!==null&&(i(),i=null),a||setTimeout(n,20)}function n(){if(!a&&!navigation.transition){var l=navigation.currentEntry;l&&l.url!=null&&navigation.navigate(l.url,{state:l.getState(),info:"react-transition",history:"replace"})}}if(typeof navigation=="object"){var a=!1,i=null;return navigation.addEventListener("navigate",e),navigation.addEventListener("navigatesuccess",t),navigation.addEventListener("navigateerror",t),setTimeout(n,100),function(){a=!0,navigation.removeEventListener("navigate",e),navigation.removeEventListener("navigatesuccess",t),navigation.removeEventListener("navigateerror",t),i!==null&&(i(),i=null)}}}function Lc(e){this._internalRoot=e}Jr.prototype.render=Lc.prototype.render=function(e){var t=this._internalRoot;if(t===null)throw Error(d(409));var n=t.current,a=Bt();Ef(n,a,e,t,null,null)},Jr.prototype.unmount=Lc.prototype.unmount=function(){var e=this._internalRoot;if(e!==null){this._internalRoot=null;var t=e.containerInfo;Ef(e.current,2,null,e,null,null),Mr(),t[qa]=null}};function Jr(e){this._internalRoot=e}Jr.prototype.unstable_scheduleHydration=function(e){if(e){var t=wu();e={blockedOn:null,target:e,priority:t};for(var n=0;n<ia.length&&t!==0&&t<ia[n].priority;n++);ia.splice(n,0,e),n===0&&qf(e)}};var zf=c.version;if(zf!=="19.3.1")throw Error(d(527,zf,"19.3.1"));U.findDOMNode=function(e){var t=e._reactInternals;if(t===void 0)throw typeof e.render=="function"?Error(d(188)):(e=Object.keys(e).join(","),Error(d(268,e)));return e=R(t),e=e!==null?x(e):null,e=e===null?null:e.stateNode,e};var dh={bundleType:0,version:"19.3.1",rendererPackageName:"react-dom",currentDispatcherRef:A,reconcilerVersion:"19.3.1"};if(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__<"u"){var $r=__REACT_DEVTOOLS_GLOBAL_HOOK__;if(!$r.isDisabled&&$r.supportsFiber)try{Ei=$r.inject(dh),At=$r}catch{}}return bl.createRoot=function(e,t){if(!j(e))throw Error(d(299));var n=!1,a="",i=S_,l=j_,s=E_;return t!=null&&(t.unstable_strictMode===!0&&(n=!0),t.identifierPrefix!==void 0&&(a=t.identifierPrefix),t.onUncaughtError!==void 0&&(i=t.onUncaughtError),t.onCaughtError!==void 0&&(l=t.onCaughtError),t.onRecoverableError!==void 0&&(s=t.onRecoverableError)),t=Sf(e,1,!1,null,null,n,a,null,i,l,s,Mf),e[qa]=t.current,vc(e),new Lc(t)},bl.hydrateRoot=function(e,t,n){if(!j(e))throw Error(d(299));var a=!1,i="",l=S_,s=j_,u=E_,_=null;return n!=null&&(n.unstable_strictMode===!0&&(a=!0),n.identifierPrefix!==void 0&&(i=n.identifierPrefix),n.onUncaughtError!==void 0&&(l=n.onUncaughtError),n.onCaughtError!==void 0&&(s=n.onCaughtError),n.onRecoverableError!==void 0&&(u=n.onRecoverableError),n.formState!==void 0&&(_=n.formState)),t=Sf(e,1,!0,t,n??null,a,i,_,l,s,u,Mf),t.context=jf(null),n=t.current,a=Bt(),a=fs(a),i=Gn(a),i.callback=null,In(n,i,a),n=a,t.current.lanes=n,Ti(t,n),fn(t),e[qa]=t.current,vc(e),new Jr(t)},bl.version="19.3.1",bl}var Pf;function Nh(){if(Pf)return Yc.exports;Pf=1;function o(){if(!(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__>"u"||typeof __REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE!="function"))try{__REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE(o)}catch(c){console.error(c)}}return o(),Yc.exports=wh(),Yc.exports}var Sh=Nh();/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const jh=o=>o.replace(/([a-z0-9])([A-Z])/g,"$1-$2").toLowerCase(),tp=(...o)=>o.filter((c,m,d)=>!!c&&c.trim()!==""&&d.indexOf(c)===m).join(" ").trim();/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */var Eh={xmlns:"http://www.w3.org/2000/svg",width:24,height:24,viewBox:"0 0 24 24",fill:"none",stroke:"currentColor",strokeWidth:2,strokeLinecap:"round",strokeLinejoin:"round"};/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const kh=Q.forwardRef(({color:o="currentColor",size:c=24,strokeWidth:m=2,absoluteStrokeWidth:d,className:j="",children:O,iconNode:N,...L},B)=>Q.createElement("svg",{ref:B,...Eh,width:c,height:c,stroke:o,strokeWidth:d?Number(m)*24/Number(c):m,className:tp("lucide",j),...L},[...N.map(([R,x])=>Q.createElement(R,x)),...Array.isArray(O)?O:[O]]));/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ce=(o,c)=>{const m=Q.forwardRef(({className:d,...j},O)=>Q.createElement(kh,{ref:O,iconNode:c,className:tp(`lucide-${jh(o)}`,d),...j}));return m.displayName=`${o}`,m};/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Th=[["path",{d:"M22 12h-2.48a2 2 0 0 0-1.93 1.46l-2.35 8.36a.25.25 0 0 1-.48 0L9.24 2.18a.25.25 0 0 0-.48 0l-2.35 8.36A2 2 0 0 1 4.49 12H2",key:"169zse"}]],ra=ce("Activity",Th);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ch=[["rect",{width:"16",height:"10",x:"2",y:"7",rx:"2",ry:"2",key:"1w10f2"}],["line",{x1:"22",x2:"22",y1:"11",y2:"13",key:"4dh1rd"}]],Ah=ce("Battery",Ch);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Oh=[["path",{d:"M20 6 9 17l-5-5",key:"1gmf2c"}]],au=ce("Check",Oh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const qh=[["path",{d:"m6 9 6 6 6-6",key:"qrunsl"}]],Rh=ce("ChevronDown",qh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Dh=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["line",{x1:"12",x2:"12",y1:"8",y2:"12",key:"1pkeuh"}],["line",{x1:"12",x2:"12.01",y1:"16",y2:"16",key:"4dfq90"}]],Mh=ce("CircleAlert",Dh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const zh=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["path",{d:"m9 12 2 2 4-4",key:"dzmm74"}]],iu=ce("CircleCheck",zh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Uh=[["rect",{width:"8",height:"4",x:"8",y:"2",rx:"1",ry:"1",key:"tgr4d6"}],["path",{d:"M16 4h2a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2",key:"116196"}],["path",{d:"m9 14 2 2 4-4",key:"df797q"}]],Bh=ce("ClipboardCheck",Uh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Vh=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["polyline",{points:"12 6 12 12 16.5 12",key:"1aq6pp"}]],Lh=ce("Clock3",Vh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Hh=[["path",{d:"m18 16 4-4-4-4",key:"1inbqp"}],["path",{d:"m6 8-4 4 4 4",key:"15zrgr"}],["path",{d:"m14.5 4-5 16",key:"e7oirm"}]],np=ce("CodeXml",Hh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Gh=[["rect",{width:"14",height:"14",x:"8",y:"8",rx:"2",ry:"2",key:"17jyea"}],["path",{d:"M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2",key:"zix9uf"}]],lu=ce("Copy",Gh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ih=[["rect",{width:"16",height:"16",x:"4",y:"4",rx:"2",key:"14l7u7"}],["rect",{width:"6",height:"6",x:"9",y:"9",rx:"1",key:"5aljv4"}],["path",{d:"M15 2v2",key:"13l42r"}],["path",{d:"M15 20v2",key:"15mkzm"}],["path",{d:"M2 15h2",key:"1gxd5l"}],["path",{d:"M2 9h2",key:"1bbxkp"}],["path",{d:"M20 15h2",key:"19e6y8"}],["path",{d:"M20 9h2",key:"19tzq7"}],["path",{d:"M9 2v2",key:"165o2o"}],["path",{d:"M9 20v2",key:"i2bqo8"}]],Aa=ce("Cpu",Ih);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Yh=[["path",{d:"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4",key:"ih7n3h"}],["polyline",{points:"7 10 12 15 17 10",key:"2ggqvy"}],["line",{x1:"12",x2:"12",y1:"15",y2:"3",key:"1vk2je"}]],Qh=ce("Download",Yh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ph=[["path",{d:"M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z",key:"1rqfz7"}],["path",{d:"M14 2v4a2 2 0 0 0 2 2h4",key:"tnqrlb"}],["path",{d:"M10 9H8",key:"b1mrlr"}],["path",{d:"M16 13H8",key:"t4e002"}],["path",{d:"M16 17H8",key:"z1uh3a"}]],Kh=ce("FileText",Ph);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Xh=[["path",{d:"M12 10a2 2 0 0 0-2 2c0 1.02-.1 2.51-.26 4",key:"1nerag"}],["path",{d:"M14 13.12c0 2.38 0 6.38-1 8.88",key:"o46ks0"}],["path",{d:"M17.29 21.02c.12-.6.43-2.3.5-3.02",key:"ptglia"}],["path",{d:"M2 12a10 10 0 0 1 18-6",key:"ydlgp0"}],["path",{d:"M2 16h.01",key:"1gqxmh"}],["path",{d:"M21.8 16c.2-2 .131-5.354 0-6",key:"drycrb"}],["path",{d:"M5 19.5C5.5 18 6 15 6 12a6 6 0 0 1 .34-2",key:"1tidbn"}],["path",{d:"M8.65 22c.21-.66.45-1.32.57-2",key:"13wd9y"}],["path",{d:"M9 6.8a6 6 0 0 1 9 5.2v2",key:"1fr1j5"}]],ru=ce("Fingerprint",Xh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Fh=[["path",{d:"M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z",key:"96xj49"}]],ns=ce("Flame",Fh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Zh=[["path",{d:"m12 14 4-4",key:"9kzdfg"}],["path",{d:"M3.34 19a10 10 0 1 1 17.32 0",key:"19p75a"}]],as=ce("Gauge",Zh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Jh=[["circle",{cx:"12",cy:"12",r:"3",key:"1v7zrd"}],["line",{x1:"3",x2:"9",y1:"12",y2:"12",key:"1dyftd"}],["line",{x1:"15",x2:"21",y1:"12",y2:"12",key:"oup4p8"}]],$h=ce("GitCommitHorizontal",Jh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Wh=[["line",{x1:"22",x2:"2",y1:"12",y2:"12",key:"1y58io"}],["path",{d:"M5.45 5.11 2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z",key:"oot6mr"}],["line",{x1:"6",x2:"6.01",y1:"16",y2:"16",key:"sgf278"}],["line",{x1:"10",x2:"10.01",y1:"16",y2:"16",key:"1l4acy"}]],Nl=ce("HardDrive",Wh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const eg=[["path",{d:"M12.83 2.18a2 2 0 0 0-1.66 0L2.6 6.08a1 1 0 0 0 0 1.83l8.58 3.91a2 2 0 0 0 1.66 0l8.58-3.9a1 1 0 0 0 0-1.83z",key:"zw3jo"}],["path",{d:"M2 12a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 12",key:"1wduqc"}],["path",{d:"M2 17a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 17",key:"kqbvx6"}]],su=ce("Layers",eg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const tg=[["circle",{cx:"12",cy:"16",r:"1",key:"1au0dj"}],["rect",{x:"3",y:"10",width:"18",height:"12",rx:"2",key:"6s8ecr"}],["path",{d:"M7 10V7a5 5 0 0 1 10 0v3",key:"1pqi11"}]],ou=ce("LockKeyhole",tg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ng=[["path",{d:"M18 8V6a2 2 0 0 0-2-2H4a2 2 0 0 0-2 2v7a2 2 0 0 0 2 2h8",key:"10dyio"}],["path",{d:"M10 19v-3.96 3.15",key:"1irgej"}],["path",{d:"M7 19h5",key:"qswx4l"}],["rect",{width:"6",height:"10",x:"16",y:"12",rx:"2",key:"1egngj"}]],ag=ce("MonitorSmartphone",ng);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ig=[["path",{d:"M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z",key:"a7tn18"}]],lg=ce("Moon",ig);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const rg=[["rect",{x:"16",y:"16",width:"6",height:"6",rx:"1",key:"4q2zg0"}],["rect",{x:"2",y:"16",width:"6",height:"6",rx:"1",key:"8cvhb9"}],["rect",{x:"9",y:"2",width:"6",height:"6",rx:"1",key:"1egb70"}],["path",{d:"M5 16v-3a1 1 0 0 1 1-1h12a1 1 0 0 1 1 1v3",key:"1jsf9p"}],["path",{d:"M12 12V8",key:"2874zd"}]],sg=ce("Network",rg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const og=[["circle",{cx:"13.5",cy:"6.5",r:".5",fill:"currentColor",key:"1okk4w"}],["circle",{cx:"17.5",cy:"10.5",r:".5",fill:"currentColor",key:"f64h9f"}],["circle",{cx:"8.5",cy:"7.5",r:".5",fill:"currentColor",key:"fotxhn"}],["circle",{cx:"6.5",cy:"12.5",r:".5",fill:"currentColor",key:"qy21gx"}],["path",{d:"M12 2C6.5 2 2 6.5 2 12s4.5 10 10 10c.926 0 1.648-.746 1.648-1.688 0-.437-.18-.835-.437-1.125-.29-.289-.438-.652-.438-1.125a1.64 1.64 0 0 1 1.668-1.668h1.996c3.051 0 5.555-2.503 5.555-5.554C21.965 6.012 17.461 2 12 2z",key:"12rzf8"}]],cg=ce("Palette",og);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ug=[["polygon",{points:"6 3 20 12 6 21 6 3",key:"1oa8hb"}]],ap=ce("Play",ug);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const dg=[["path",{d:"M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8",key:"v9h5vc"}],["path",{d:"M21 3v5h-5",key:"1q7to0"}],["path",{d:"M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16",key:"3uifl3"}],["path",{d:"M8 16H3v5",key:"1cv678"}]],cu=ce("RefreshCw",dg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const _g=[["path",{d:"M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8",key:"1357e3"}],["path",{d:"M3 3v5h5",key:"1xhq8a"}]],ip=ce("RotateCcw",_g);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const mg=[["circle",{cx:"11",cy:"11",r:"8",key:"4ej97u"}],["path",{d:"m21 21-4.3-4.3",key:"1qie3q"}]],lp=ce("Search",mg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const fg=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}],["path",{d:"M12 8v4",key:"1got3b"}],["path",{d:"M12 16h.01",key:"1drbdi"}]],Si=ce("ShieldAlert",fg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const pg=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}],["path",{d:"m9 12 2 2 4-4",key:"dzmm74"}]],On=ce("ShieldCheck",pg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const vg=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}]],yg=ce("Shield",vg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const hg=[["line",{x1:"4",x2:"4",y1:"21",y2:"14",key:"1p332r"}],["line",{x1:"4",x2:"4",y1:"10",y2:"3",key:"gb41h5"}],["line",{x1:"12",x2:"12",y1:"21",y2:"12",key:"hf2csr"}],["line",{x1:"12",x2:"12",y1:"8",y2:"3",key:"1kfi7u"}],["line",{x1:"20",x2:"20",y1:"21",y2:"16",key:"1lhrwl"}],["line",{x1:"20",x2:"20",y1:"12",y2:"3",key:"16vvfq"}],["line",{x1:"2",x2:"6",y1:"14",y2:"14",key:"1uebub"}],["line",{x1:"10",x2:"14",y1:"8",y2:"8",key:"1yglbp"}],["line",{x1:"18",x2:"22",y1:"16",y2:"16",key:"1jxqpz"}]],gg=ce("SlidersVertical",hg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const bg=[["rect",{width:"14",height:"20",x:"5",y:"2",rx:"2",ry:"2",key:"1yt0o3"}],["path",{d:"M12 18h.01",key:"mhygvu"}]],xg=ce("Smartphone",bg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const wg=[["path",{d:"M9.937 15.5A2 2 0 0 0 8.5 14.063l-6.135-1.582a.5.5 0 0 1 0-.962L8.5 9.936A2 2 0 0 0 9.937 8.5l1.582-6.135a.5.5 0 0 1 .963 0L14.063 8.5A2 2 0 0 0 15.5 9.937l6.135 1.581a.5.5 0 0 1 0 .964L15.5 14.063a2 2 0 0 0-1.437 1.437l-1.582 6.135a.5.5 0 0 1-.963 0z",key:"4pj2yx"}],["path",{d:"M20 3v4",key:"1olli1"}],["path",{d:"M22 5h-4",key:"1gvqau"}],["path",{d:"M4 17v2",key:"vumght"}],["path",{d:"M5 18H3",key:"zchphs"}]],Ng=ce("Sparkles",wg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Sg=[["circle",{cx:"12",cy:"12",r:"4",key:"4exip2"}],["path",{d:"M12 2v2",key:"tus03m"}],["path",{d:"M12 20v2",key:"1lh1kg"}],["path",{d:"m4.93 4.93 1.41 1.41",key:"149t6j"}],["path",{d:"m17.66 17.66 1.41 1.41",key:"ptbguv"}],["path",{d:"M2 12h2",key:"1t8f8n"}],["path",{d:"M20 12h2",key:"1q8mjw"}],["path",{d:"m6.34 17.66-1.41 1.41",key:"1m8zz5"}],["path",{d:"m19.07 4.93-1.41 1.41",key:"1shlcs"}]],jg=ce("Sun",Sg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Eg=[["polyline",{points:"4 17 10 11 4 5",key:"akl6gq"}],["line",{x1:"12",x2:"20",y1:"19",y2:"19",key:"q2wloq"}]],uu=ce("Terminal",Eg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const kg=[["path",{d:"M14 4v10.54a4 4 0 1 1-4 0V4a2 2 0 0 1 4 0Z",key:"17jzev"}]],rp=ce("Thermometer",kg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Tg=[["path",{d:"m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3",key:"wmoenq"}],["path",{d:"M12 9v4",key:"juzpu7"}],["path",{d:"M12 17h.01",key:"p32p05"}]],Cg=ce("TriangleAlert",Tg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ag=[["path",{d:"M18 6 6 18",key:"1bl5f8"}],["path",{d:"m6 6 12 12",key:"d8bk6v"}]],sp=ce("X",Ag);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Og=[["path",{d:"M4 14a1 1 0 0 1-.78-1.63l9.9-10.2a.5.5 0 0 1 .86.46l-1.92 6.02A1 1 0 0 0 13 10h7a1 1 0 0 1 .78 1.63l-9.9 10.2a.5.5 0 0 1-.86-.46l1.92-6.02A1 1 0 0 0 11 14z",key:"1xq2db"}]],qg=ce("Zap",Og),op="coreflow.appearance.v1",Rg=["system","light","dark"],cp=["ocean","emerald","amber"];function Dg(){var o;try{const c=typeof window<"u"?(o=window.localStorage)==null?void 0:o.getItem(op):null,m=JSON.parse(c||"{}"),d=Rg.includes(m.theme)?m.theme:"system",j=cp.includes(m.accent)?m.accent:"ocean";return{theme:d,reduceMotion:m.reduceMotion===!0,accent:j}}catch{return{theme:"system",reduceMotion:!1,accent:"ocean"}}}function up(o){var c;return o==="dark"?!0:o==="light"?!1:typeof window<"u"&&!!((c=window.matchMedia)!=null&&c.call(window,"(prefers-color-scheme: dark)").matches)}function dp(o){if(typeof document>"u")return;const c=up(o.theme),m=o.accent&&cp.includes(o.accent)?o.accent:"ocean";document.documentElement.dataset.theme=c?"dark":"light",document.documentElement.dataset.motion=o.reduceMotion?"reduce":"system",document.documentElement.dataset.accent=m}function Kf(o){dp(o);let c=!0;try{window.localStorage.setItem(op,JSON.stringify(o))}catch{c=!1}return typeof window<"u"&&window.dispatchEvent(new CustomEvent("coreflow-appearance",{detail:o})),c}const Xf=1e4,Mg=12e4;let Ff=0;function ls(){var o;return typeof window<"u"&&typeof((o=window.ksu)==null?void 0:o.exec)=="function"}function Xc(o){if(typeof o=="string")return{errno:0,stdout:o,stderr:""};if(typeof o=="object"&&o!==null){const c=o,m=typeof c.errno=="number"&&Number.isFinite(c.errno)?c.errno:-1,d=typeof c.stdout=="string"?c.stdout:"",j=typeof c.stderr=="string"?c.stderr:"";return{errno:m,stdout:d,stderr:j}}return{errno:-1,stdout:"",stderr:"Malformed native bridge response"}}async function ji(o,c=Xf){if(!ls()||!window.ksu)return{errno:-1,stdout:"",stderr:"Native KernelSU bridge unavailable"};if(typeof o!="string"||!o||o.includes("\0"))return{errno:-1,stdout:"",stderr:"Invalid shell command"};const m=window.ksu,d=Number.isFinite(c)?Math.min(Mg,Math.max(1e3,c)):Xf;return new Promise(j=>{let O=!1;Ff+=1;const N=`__coreflow_ksu_cb_${Date.now()}_${Ff}`,L=R=>{O||(O=!0,window.clearTimeout(B),delete window[N],j(R))},B=window.setTimeout(()=>{L({errno:-1,stdout:"",stderr:`Native command timed out after ${d}ms`})},d);window[N]=(R,x,g)=>{if(typeof R=="object"&&R!==null){L(Xc(R));return}L({errno:typeof R=="number"&&Number.isFinite(R)?R:0,stdout:typeof x=="string"?x:String(x??""),stderr:typeof g=="string"?g:String(g??"")})};try{const R=m.exec(o,"{}",N);R!==void 0&&Promise.resolve(R).then(x=>L(Xc(x))).catch(x=>L({errno:-1,stdout:"",stderr:x instanceof Error?x.message:String(x)}))}catch{try{const R=m.exec(o);Promise.resolve(R).then(x=>L(Xc(x))).catch(x=>L({errno:-1,stdout:"",stderr:x instanceof Error?x.message:String(x)}))}catch(R){L({errno:-1,stdout:"",stderr:R instanceof Error?R.message:String(R)})}}})}const $c={mode:"disabled",enableSkiavkThreaded:!1,enableAdaptiveVrs:!1,enablePipelineCachePrewarm:!1};function wl(o,c){const m=o.mode==="full_capability_override"?"safe_hardware":o.mode,d=m!=="disabled",j=(c==null?void 0:c.deviceExtensions)??0,O=(c==null?void 0:c.hwRayTracing)??(c==null?void 0:c.rayTracing)??!1,N=(c==null?void 0:c.hwMeshShaders)??(c==null?void 0:c.meshShaders)??!1,L=(c==null?void 0:c.hwVariableRateShading)??(c==null?void 0:c.variableRateShading)??!1,B=(c==null?void 0:c.hwCooperativeMatrix)??(c==null?void 0:c.cooperativeMatrix)??!1,R=(c==null?void 0:c.hwRtPipeline)??(c==null?void 0:c.rtPipeline)??!1,x=(c==null?void 0:c.hwRayQuery)??(c==null?void 0:c.rayQuery)??!1,g=(c==null?void 0:c.hwAccelerationStructure)??(c==null?void 0:c.accelerationStructure)??!1,q=(c==null?void 0:c.hwShaderFloat64)??(c==null?void 0:c.shaderFloat64)??!1,$=(c==null?void 0:c.hwIndexTypeUint8)??(c==null?void 0:c.indexTypeUint8)??!1,M=(c==null?void 0:c.hwLineRasterization)??(c==null?void 0:c.lineRasterization)??!1,X=(c==null?void 0:c.hwPushDescriptors)??(c==null?void 0:c.pushDescriptors)??!1,I=(c==null?void 0:c.hwGlobalPriorityQuery)??(c==null?void 0:c.globalPriorityQuery)??!1,J=(c==null?void 0:c.hwShaderFloatControls2)??(c==null?void 0:c.shaderFloatControls2)??!1,ae=(c==null?void 0:c.hwMaintenance5)??(c==null?void 0:c.maintenance5)??!1,F=(c==null?void 0:c.hwMaintenance6)??(c==null?void 0:c.maintenance6)??!1,re=(c==null?void 0:c.hwMaintenance7)??(c==null?void 0:c.maintenance7)??!1,xe=(c==null?void 0:c.hwDescriptorBuffer)??(c==null?void 0:c.descriptorBuffer)??!1,C=(c==null?void 0:c.hwGraphicsPipelineLibrary)??(c==null?void 0:c.graphicsPipelineLibrary)??!1,z=(c==null?void 0:c.apiVersion)??"Probing ICD...";return{available:(c==null?void 0:c.available)??!1,layerInstalled:(c==null?void 0:c.layerInstalled)??!0,layerActive:d,mode:m,vendor:(c==null?void 0:c.vendor)??"Auto-Detecting GPU Vendor",deviceName:(c==null?void 0:c.deviceName)??"Probing Physical GPU ICD...",apiVersion:z,effectiveApiVersion:z,driverVersion:(c==null?void 0:c.driverVersion)??"0.0.0",deviceExtensions:j,effectiveDeviceExtensions:j,instanceExtensions:(c==null?void 0:c.instanceExtensions)??0,deviceUuid:(c==null?void 0:c.deviceUuid)??"00000000-0000-0000-0000-000000000000",driverUuid:(c==null?void 0:c.driverUuid)??"00000000-0000-0000-0000-000000000000",pipelineCacheUuid:(c==null?void 0:c.pipelineCacheUuid)??"00000000-0000-0000-0000-000000000000",rayTracing:O,meshShaders:N,variableRateShading:L,cooperativeMatrix:B,rtPipeline:R,rayQuery:x,accelerationStructure:g,portabilitySubset:(c==null?void 0:c.portabilitySubset)??!1,robustBufferAccess:(c==null?void 0:c.robustBufferAccess)??!0,samplerAnisotropy:(c==null?void 0:c.samplerAnisotropy)??!0,shaderInt16:(c==null?void 0:c.shaderInt16)??!0,shaderFloat64:q,protectedMemory:(c==null?void 0:c.protectedMemory)??!0,timelineSemaphore:(c==null?void 0:c.timelineSemaphore)??!0,descriptorIndexing:(c==null?void 0:c.descriptorIndexing)??!0,bufferDeviceAddress:(c==null?void 0:c.bufferDeviceAddress)??!0,dynamicRendering:(c==null?void 0:c.dynamicRendering)??!0,synchronization2:(c==null?void 0:c.synchronization2)??!0,indexTypeUint8:$,lineRasterization:M,pushDescriptors:X,globalPriorityQuery:I,shaderFloatControls2:J,maxPushDescriptors:(c==null?void 0:c.maxPushDescriptors)??(X?32:0),maintenance5:ae,maintenance6:F,maintenance7:re,descriptorBuffer:xe,graphicsPipelineLibrary:C,hwRayTracing:O,hwMeshShaders:N,hwVariableRateShading:L,hwCooperativeMatrix:B,hwRtPipeline:R,hwRayQuery:x,hwAccelerationStructure:g,hwShaderFloat64:q,hwIndexTypeUint8:$,hwLineRasterization:M,hwPushDescriptors:X,hwGlobalPriorityQuery:I,hwShaderFloatControls2:J,hwMaintenance5:ae,hwMaintenance6:F,hwMaintenance7:re,hwDescriptorBuffer:xe,hwGraphicsPipelineLibrary:C,skiavkActive:d&&o.enableSkiavkThreaded,adaptiveVrsActive:d&&o.enableAdaptiveVrs&&L,pipelineCachePrewarmActive:d&&o.enablePipelineCachePrewarm,gpuKernelAvailable:(c==null?void 0:c.gpuKernelAvailable)??!1,gpuKernelLabel:(c==null?void 0:c.gpuKernelLabel)??"Universal Multi-Vendor GPU Devfreq",gpuGovernor:(c==null?void 0:c.gpuGovernor)??"auto",gpuCurrentFreqMhz:(c==null?void 0:c.gpuCurrentFreqMhz)??0,gpuMaxFreqMhz:(c==null?void 0:c.gpuMaxFreqMhz)??0,adrenoboostLevel:(c==null?void 0:c.adrenoboostLevel)??0}}const Zf={version:"general",log_level:"general",zero_polling_mode:"general",enable_cgroups_v2:"general",enable_uclamp:"general",enable_tuner:"general",tier_override:"device_adaptation",unlock_vendor_clamping:"device_adaptation",force_120hz_rendering:"device_adaptation",enable_cpu_frequency_control:"device_adaptation",enable_gpu_frequency_control:"device_adaptation",enable_direct_adaptive:"direct_adaptive",direct_adaptive_policy:"direct_adaptive",top_app_interactive_min:"uclamp_tuning",top_app_interactive_max:"uclamp_tuning",ui_adaptive_bias:"uclamp_tuning",ui_balanced_uclamp_min:"uclamp_tuning",foreground_boost_min:"uclamp_tuning",background_clamp_max:"uclamp_tuning",system_background_max:"uclamp_tuning",temp_monitor_interval_ms:"thermal_guardian",thermal_trip_celsius:"thermal_guardian",thermal_recover_celsius:"thermal_guardian",emergency_rollback_on_overheat:"thermal_guardian",cpu_pressure_boost_pct:"stability_policy",memory_pressure_guard_pct:"stability_policy",io_pressure_guard_pct:"stability_policy",thermal_headroom_celsius:"stability_policy",thermal_emergency_margin_celsius:"stability_policy",enable_pid_equilibrium:"pid_equilibrium",pid_target_celsius:"pid_equilibrium",pid_kp:"pid_equilibrium",pid_ki:"pid_equilibrium",pid_kd:"pid_equilibrium",enable_battery_step_charging:"hardware_longevity",enable_ufs_burst_pacing:"hardware_longevity",battery_step_temp_threshold:"hardware_longevity",stepped_charge_current_ma:"hardware_longevity",top_app_io_weight:"hardware_longevity",background_io_weight:"hardware_longevity",vulkan_layer_mode:"vulkan_engine",enable_skiavk_threaded:"vulkan_engine",enable_adaptive_vrs:"vulkan_engine",enable_pipeline_cache_prewarm:"vulkan_engine",target_fps:"render_booster",pre_boost_on_touch:"render_booster",boost_duration_ms:"render_booster"};function Ni(o){return Object.prototype.hasOwnProperty.call(Zf,o)?Zf[o]:void 0}function _p(o){const c=o.trim();return c==="stability"?"stability_policy":c}function zg(o){const c=Object.create(null);let m="";for(const d of o.split(/\r?\n/)){const j=d.trim();if(!j||j.startsWith("#")||j.startsWith(";"))continue;if(j.startsWith("[")&&j.endsWith("]")){m=_p(j.slice(1,-1)),c[m]||(c[m]=Object.create(null));continue}const O=j.indexOf("=");if(O<0)continue;const N=j.slice(0,O).trim(),L=j.slice(O+1).trim();if(!N)continue;const B=Ni(N);if(B&&m!==B)continue;const R=m||B||"general";c[R]||(c[R]=Object.create(null)),c[R][N]=L}return c}function Wc(o,c){var j;const m=zg(o),d=Ni(c);if(d){const O=(j=m[d])==null?void 0:j[c];return O&&O.length>0?O:null}for(const O of Object.values(m))if(O[c])return O[c];return null}function Ug(o,c){const m=Wc(o,c);if(m===null)return null;const d=Number(m);return Number.isFinite(d)?d:null}function Bg(o,c){const m={};for(const[B,R]of Object.entries(c)){const x=B.trim();if(!x||Ni(x)===void 0)continue;const g=String(R).trim();/[\r\n\0]/.test(g)||(m[x]=g)}if(Object.keys(m).length===0)return o;const d=o?o.replace(/\r\n/g,`
`).split(`
`):[];d.length>0&&d[d.length-1]===""&&d.pop();const j=[];let O="";const N=B=>{if(B)for(const[R,x]of Object.entries(m))Ni(R)===B&&(j.push(`${R} = ${x}`),delete m[R])};for(const B of d){const R=B.trim();if(R.startsWith("[")&&R.endsWith("]")){N(O),O=_p(R.slice(1,-1)),j.push(B);continue}if(!R||R.startsWith("#")||R.startsWith(";")){j.push(B);continue}const x=B.indexOf("=");if(x<0){j.push(B);continue}const g=B.slice(0,x).trim();Object.prototype.hasOwnProperty.call(m,g)&&Ni(g)===O?(j.push(`${g} = ${m[g]}`),delete m[g]):j.push(B)}N(O);const L=["general","device_adaptation","direct_adaptive","uclamp_tuning","thermal_guardian","stability_policy","pid_equilibrium","hardware_longevity","vulkan_engine","render_booster"];for(const B of L){const R=Object.keys(m).filter(x=>Ni(x)===B);if(R.length!==0){j.length>0&&j[j.length-1].trim()!==""&&j.push(""),j.push(`[${B}]`);for(const x of R)j.push(`${x} = ${m[x]}`),delete m[x]}}return j.join(`
`)+`
`}const Fc=o=>typeof o=="object"&&o!==null&&!Array.isArray(o);function Vg(o){if(!Fc(o))return!1;const c=["cpuUtilizationPct","thermalPressure","ramBytes","zramBytes","refreshHz","discoveredNodes","baselineNodes","verifiedMutations","skippedNodes","ownershipLosses"],m=["mode","screenState","gpuModel","storageType"];return!(typeof o.active!="boolean"||typeof o.discoveryComplete!="boolean"||typeof o.thermalGuard!="boolean"||typeof o.journalFault!="boolean"||!c.every(d=>typeof o[d]=="number"&&Number.isFinite(o[d]))||!m.every(d=>typeof o[d]=="string")||!Array.isArray(o.cpuClusters)||!o.cpuClusters.every(d=>Fc(d)&&typeof d.id=="string"&&Array.isArray(d.opps)&&d.opps.every(j=>typeof j=="string"))||!Array.isArray(o.appliedValues)||!o.appliedValues.every(d=>Fc(d)&&["semantic","domain","path","current","oemBaseline","target","health"].every(j=>typeof d[j]=="string")&&typeof d.modified=="boolean"&&typeof d.owned=="boolean"))}const Sl={thermalTripCelsius:44,balancedUclampMin:320};function du(o){switch(o){case"sustained_load":return{label:"Sustained Load",description:"Coordinated CPU responsiveness transaction active with verified thermal headroom.",stage:"OBSERVING",risk:"Medium",outcome:"IMPROVED"};case"pressure_guard":return{label:"Pressure Guard",description:"Memory or I/O contention elevated; dynamic block scheduler & VM relief engaged.",stage:"OBSERVING",risk:"Low",outcome:"IMPROVED"};case"thermal_guard":return{label:"Thermal Guard",description:"OEM/Android thermal safety gate engaged; active mitigation pacing SoC demand.",stage:"VERIFIED",risk:"Low",outcome:"ABORTED"};case"balanced":default:return{label:"Balanced Equilibrium",description:"Coordinated daily baseline retained with readback-verified top-app UClamp floor.",stage:"RETAINED",risk:"Low",outcome:"IMPROVED"}}}function Lg(){const o=du("balanced");return{mode:"balanced",modeLabel:o.label,modeDescription:o.description,engineActive:!0,uclampMin:320,uclampMax:1024,uclampSupported:!0,uclampPolicyMin:320,uclampPolicyMax:1024,uclampTargetPath:"/dev/cpuctl/top-app",uclampEncoding:"percentage",platformVendor:"Universal Android",osFamily:"Android AOSP / GKI",osMajor:15,executionTier:"UniversalFallback",policyFamily:"CoreFlowBase",identityConfidence:"High",tier1Eligible:!0,inspectionMode:!1,mutationRuntimeStarted:!0,socTempCelsius:38.6,socCompositeCelsius:38.6,cpuAverageCelsius:38.2,cpuPeakCelsius:41.5,gpuTempCelsius:36.8,batteryTempCelsius:33.4,skinTempCelsius:35.1,thermalSource:"android+sysfs-oem-trips",thermalSensor:"SoC-Composite (CPU+GPU)",thermalSeverity:0,thermalPressure:44,thermalSensors:[{id:"tz0",type:"cpu-0-0",category:1,celsius:37.8,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.02},{id:"tz1",type:"cpu-0-1",category:1,celsius:38,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.01},{id:"tz2",type:"cpu-1-0",category:1,celsius:39.4,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.04},{id:"tz3",type:"cpu-1-2",category:1,celsius:41.5,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.05},{id:"tz4",type:"gpuss-0",category:1,celsius:36.8,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:-.02},{id:"tz5",type:"quiet_therm",category:3,celsius:35.1,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:0},{id:"tz6",type:"battery",category:2,celsius:33.4,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:0},{id:"tz7",type:"ap_therm",category:4,celsius:37,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.01}],cpuPressurePct:1.2,memoryPressurePct:.3,ioPressurePct:.4,cpuPressureValid:!0,memoryPressureValid:!0,ioPressureValid:!0,baselineNodesSnapshotted:6,baselineStatus:"protected",transactionStage:"RETAINED",riskClass:"Low",lastOutcome:"IMPROVED",ownershipIntact:!0,aiBackend:"embedded-rls-mlp",aiOnnxActive:!1,aiPredictedPressure10s:46.2,aiPredictedVelocity:.08,aiRecommendedScale:.94,aiConfidence:.86,aiSamples:12,tunerRuntimeStage:"STAGE_ACTIVE_TUNING",tunerStage:"RETAINED",tunerIntent:"HoldEquilibrium",tunerWorkload:"GeneralInteraction",tunerBottleneck:"None",tunerObjective:"HoldBaseline",tunerCapabilityFingerprint:"cddp-v4:8f4e2b91c073a612",tunerCandidateId:"baseline",tunerCandidateLabel:"OEM Verified Baseline",tunerRationale:"System in equilibrium (I/O & memory PSI low); retaining OEM block scheduler without unnecessary mutation.",tunerDecisionPlanRationale:"Global candidate pool evaluated; equilibrium held with zero conflict rejections.",tunerVerdict:"IMPROVED",tunerUtilityDelta:0,tunerNoiseFloor:.05,tunerTrials:3,tunerImproved:2,tunerRolledBack:1,tunerInconclusive:0,tunerRejected:0,tunerThermalStabilization:!1,tunerThermalRecovery:!1,blockQueues:[{device:"sda",activeScheduler:"mq-deadline",baselineScheduler:"none",availableSchedulers:["none","mq-deadline","kyber","bfq"],readAheadKb:128,nrRequests:128,rqAffinity:2,readExpireMs:250,writesStarved:3,fifoBatch:8}],tunerCandidates:[{id:"io.mq_deadline_low_latency",label:"mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",domain:"io",trials:2,successes:2,rollbacks:0,meanDelta:7.4,ucbScore:8.12,cooldownTicks:0},{id:"io.kyber_token_bucket",label:"kyber Token-Bucket Low-Latency I/O Scheduler",domain:"io",trials:1,successes:0,rollbacks:1,meanDelta:-1.8,ucbScore:.42,cooldownTicks:0},{id:"cpufreq.schedutil_interactive_response",label:"EAS Governor Rate-Limit Pacing (500us up / 10000us down)",domain:"cpufreq",trials:1,successes:1,rollbacks:0,meanDelta:5.2,ucbScore:6.18,cooldownTicks:0},{id:"vm.zram_latency_relief",label:"VM Page-Cluster Zero-Latency & VFS Dentry Preservation",domain:"vm",trials:1,successes:1,rollbacks:0,meanDelta:4.1,ucbScore:5.15,cooldownTicks:0}],vulkan:wl($c,{available:!0,layerInstalled:!0,vendor:"Multi-Vendor Preview (Adreno / Mali / Xclipse)",deviceName:"Universal Mobile GPU Preview Profile",apiVersion:"1.3.280",driverVersion:"512.762.0",deviceExtensions:148,instanceExtensions:16,deviceUuid:"cf000001-2026-4000-a000-000000000310",driverUuid:"cf000002-2026-4000-a000-000000000310",pipelineCacheUuid:"cf000003-2026-4000-a000-000000000310",hwVariableRateShading:!0,hwPushDescriptors:!0,hwMaintenance5:!0,hwMaintenance6:!0,gpuKernelAvailable:!0,gpuKernelLabel:"Universal GPU Devfreq Surface (Preview)",gpuGovernor:"msm-adreno-tz / simple_ondemand",gpuCurrentFreqMhz:680,gpuMaxFreqMhz:1050,adrenoboostLevel:1}),timestamp:Date.now()}}function Hg(){const o=new Date,c=m=>new Date(o.getTime()-m*1e3).toTimeString().slice(0,8);return[{id:"preview-1",timestamp:c(9),type:"INFO",tag:"AdmissionGate",message:"Preview: capability semantics, range bounds, and OEM ownership verified."},{id:"preview-2",timestamp:c(6),type:"BASELINE",tag:"Coordinator",message:"Preview: write-ahead transaction journal committed prior to mutation."},{id:"preview-3",timestamp:c(3),type:"INFO",tag:"AutonomousTuner",message:"Preview: sda queue/scheduler switched none -> mq-deadline (read_expire 0.5x, fifo_batch 0.5x) | delta_u=+7.4 -> IMPROVED."}]}function Gg(o,c,m){if(!o.engineActive)return{next:{...o,uclampMin:0,uclampMax:1024,mode:"balanced",modeLabel:"Engine Standby",modeDescription:"Coordinated mutations paused; OEM baseline values intact.",transactionStage:"PLANNED",timestamp:Date.now()}};const d=Number.isFinite(c.thermalTripCelsius)?c.thermalTripCelsius:Sl.thermalTripCelsius,O=Number.isFinite(c.balancedUclampMin)?c.balancedUclampMin:Sl.balancedUclampMin,N=Math.max(0,Math.min(1024,O)),L=Number.isFinite(o.socTempCelsius)?o.socTempCelsius:38.6,B=Math.max(32,Math.min(49,L+(Math.random()-.46)*.35)),R=m==="cpu"?14:Math.max(0,Math.random()*10),x=m==="memory"?6.4:Math.max(0,Math.random()*3.5),g=m==="io_trial"?11.8:m==="memory"?6.2:Math.max(0,Math.random()*5.2);let q="balanced",$=N,M=1024;B>=d?(q="thermal_guard",$=0,M=896):x>=3||g>=5?(q="pressure_guard",$=Math.floor(N/4)):R>=8&&B<=d-3&&(q="sustained_load",$=Math.min(1024,N+Math.floor(N/2)));const X=du(q),I=Math.min(130,Math.max(25,(B-20)*2.2+R*.6+g*.4)),J=Number(((I-(o.aiPredictedPressure10s??45))*.18).toFixed(2)),ae=q==="thermal_guard"?.68:q==="pressure_guard"?.82:Number(Math.max(.72,Math.min(1,1-Math.max(0,I-55)*.004)).toFixed(2)),F=(o.aiSamples??12)+1,re=o.blockQueues??[],xe=(o.tunerCandidates??[]).map(Z=>({...Z}));let C=o.tunerStage??"RETAINED",z=o.tunerIntent??"HoldEquilibrium",P=o.tunerCandidateId??"baseline",ye=o.tunerCandidateLabel??"OEM Verified Baseline",Re=o.tunerRationale??"",Me=o.tunerVerdict??"IMPROVED",Ne=o.tunerUtilityDelta??0,$e=o.tunerTrials??3,it=o.tunerImproved??2;const Le=o.tunerRolledBack??1,H=re.map(Z=>({...Z}));if(m==="io_trial"||g>=5&&m!=="memory"){C="RETAINED",z="IoLatencyRelief",P="io.mq_deadline_low_latency",ye="mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",Me="IMPROVED",Ne=Number((5.2+Math.random()*4.5).toFixed(2)),$e+=1,it+=1,Re=`I/O PSI=${g.toFixed(1)}% exceeded threshold; switched sda scheduler none -> mq-deadline (read_expire=0.5x, writes_starved=1.5x, fifo_batch=0.5x, read_ahead_kb=2.0x) | Before/After ΔU=+${Ne} -> RETAINED.`,H.length>0&&(H[0].activeScheduler="mq-deadline",H[0].readExpireMs=250,H[0].writesStarved=3,H[0].fifoBatch=8,H[0].readAheadKb=256,H[0].rqAffinity=2);const Z=xe.find(Ee=>Ee.id==="io.mq_deadline_low_latency");Z&&(Z.trials+=1,Z.successes+=1,Z.meanDelta=Number(((Z.meanDelta*(Z.trials-1)+Ne)/Z.trials).toFixed(2)),Z.ucbScore=Number((Z.meanDelta+.65).toFixed(2)))}else m==="memory"||x>=3.2?(C="RETAINED",z="MemoryStallRelief",P="vm.zram_latency_relief",ye="VM Page-Cluster Zero-Latency & VFS Dentry Preservation",Me="IMPROVED",Ne=Number((3.4+Math.random()*2.8).toFixed(2)),$e+=1,it+=1,Re=`Memory PSI=${x.toFixed(1)}% elevated; applied vm.page-cluster=0 & vfs_cache_pressure=80 | Before/After ΔU=+${Ne} -> RETAINED.`):q==="thermal_guard"?(C="OBSERVING",z="ThermalStabilization",P="thermal.schedutil_damped_pacing",ye="Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)",Me="IMPROVED",Ne=Number((2.8+Math.random()*1.9).toFixed(2)),$e+=1,it+=1,Re=`Thermal headroom constrained (${B.toFixed(1)}°C); switched objective to ThermalStabilization and damped EAS governor ramp rate (2.0x up / 0.5x down relative to OEM baseline).`):(m==="cpu"||R>=8.5)&&(C="RETAINED",z="InteractiveCpuPacing",P="cpufreq.schedutil_interactive_response",ye="EAS Governor Rate-Limit Pacing (500us up / 10000us down)",Me="IMPROVED",Ne=Number((4.1+Math.random()*2.5).toFixed(2)),$e+=1,it+=1,Re=`CPU PSI=${R.toFixed(1)}% with thermal headroom; paced EAS schedutil governor rate limits (up=500us, down=10000us) | Before/After ΔU=+${Ne} -> RETAINED.`);const ne={...o,mode:q,modeLabel:X.label,modeDescription:X.description,uclampMin:$,uclampMax:M,socTempCelsius:Number(B.toFixed(1)),cpuPressurePct:Number(R.toFixed(1)),memoryPressurePct:Number(x.toFixed(1)),ioPressurePct:Number(g.toFixed(1)),transactionStage:X.stage,riskClass:X.risk,lastOutcome:X.outcome,ownershipIntact:!0,aiBackend:o.aiBackend??"embedded-rls-mlp",aiOnnxActive:o.aiOnnxActive??!1,aiPredictedPressure10s:Number(I.toFixed(1)),aiPredictedVelocity:J,aiRecommendedScale:ae,aiConfidence:Number(Math.min(.96,.78+Math.min(F,50)*.003).toFixed(2)),aiSamples:F,tunerStage:C,tunerIntent:z,tunerCandidateId:P,tunerCandidateLabel:ye,tunerRationale:Re,tunerVerdict:Me,tunerUtilityDelta:Ne,tunerTrials:$e,tunerImproved:it,tunerRolledBack:Le,blockQueues:H,tunerCandidates:xe,timestamp:Date.now()};return q===o.mode&&!m?{next:ne}:{next:ne,event:Ig(q,X.label,ne,m)}}let Zc=0;function Ig(o,c,m,d){return Zc+=1,d==="io_trial"?{id:`preview-${Date.now()}-${Zc}`,timestamp:new Date().toTimeString().slice(0,8),type:"BOOST",tag:"AutonomousTuner",message:m.tunerRationale??"Dynamic scheduler trial completed."}:{id:`preview-${Date.now()}-${Zc}`,timestamp:new Date().toTimeString().slice(0,8),type:o==="thermal_guard"?"THERMAL":o==="sustained_load"?"BOOST":"INFO",tag:"Transaction",message:`Stage=${m.transactionStage} -> ${c} | uclamp=${m.uclampMin}/${m.uclampMax} | cpu=${m.cpuPressurePct}% mem=${m.memoryPressurePct}% io=${m.ioPressurePct}% temp=${m.socTempCelsius}C`}}function mp(o,c=Sl.thermalTripCelsius){var F,re,xe,C,z;const m=Number.isFinite(c)&&c>25?c:44,d=Number.isFinite(o.socTempCelsius)?o.socTempCelsius:-1,j=o.cpuPressureValid!==!1&&Number.isFinite(o.cpuPressurePct)?Math.max(0,o.cpuPressurePct):0,O=o.memoryPressureValid!==!1&&Number.isFinite(o.memoryPressurePct)?Math.max(0,o.memoryPressurePct):0,N=o.ioPressureValid!==!1&&Number.isFinite(o.ioPressurePct)?Math.max(0,o.ioPressurePct):0,L=!!o.thermalGuardActive||!!((F=o.directAdaptive)!=null&&F.thermalGuard)||o.mode==="thermal_guard"||d>0&&d>=m,B=!L&&(d>0&&d>=m-2.5||typeof o.thermalPressure=="number"&&Number.isFinite(o.thermalPressure)&&o.thermalPressure>=60),R=L?"guard":B?"elevated":"optimal",x=Math.max(j*.65,O*1.8,N*1.4),g=x>=10||O>=5||N>=8?"critical":x>=4||O>=2.2||N>=3.5?"contended":"calm",q=!!((re=o.directAdaptive)!=null&&re.journalFault),$=((xe=o.directAdaptive)==null?void 0:xe.ownershipLosses)??0,M=q?"fault":o.ownershipIntact===!1||$>0?"contended":"intact";let X=100;o.engineActive||(X-=12),R==="guard"?X-=24:R==="elevated"&&(X-=10),d>36&&(X-=Math.min(14,Math.round((d-36)*1.4))),X-=Math.min(24,Math.round(j*.7+O*2.1+N*1.5)),M==="fault"?X-=35:M==="contended"&&(X-=14);const I=Math.max(15,Math.min(100,X)),J=new Set;if(o.uclampSupported&&J.add("UClamp"),(C=o.directAdaptive)!=null&&C.active&&J.add("DirectAdaptive"),o.tunerActiveInterventions&&o.tunerActiveInterventions.length>0)for(const P of o.tunerActiveInterventions)P.domain&&J.add(P.domain);else if(o.tunerCandidateId&&o.tunerCandidateId!=="baseline"){const P=o.tunerCandidateId.split(".")[0];P&&J.add(P.toUpperCase())}(z=o.vulkan)!=null&&z.layerActive&&J.add("VulkanLayer");const ae=q?"Journal fault latched — mutations halted while OEM recovery remains protected.":R==="guard"?"Thermal Guard engaged — pacing SoC demand within OEM thermal boundaries.":g==="critical"?"Resource contention detected — coordinated relief transaction active.":o.engineActive?"System in verified equilibrium — readback-checked policies active.":"Runtime in standby — OEM baseline values intact.";return{equilibriumScore:I,thermalTone:R,pressureTone:g,ownershipTone:M,headline:ae,activeDomains:Array.from(J)}}const Jc=[{key:"PLANNED",label:"Planned"},{key:"APPLIED",label:"Applied"},{key:"VERIFIED",label:"Readback Verified"},{key:"OBSERVING",label:"Observing"},{key:"RETAINED",label:"Retained"}],Yg=({telemetry:o,nativeMode:c=!1,thermalTripCelsius:m,onTriggerSimulation:d})=>{var xe;const j=du(o.mode),O=mp(o,m),N=o.socTempCelsius,L=c&&N<0?"N/A":N.toFixed(1),B=o.tunerStage??o.transactionStage??j.stage,R=o.engineActive?Jc.some(C=>C.key===B)?B:"RETAINED":"PLANNED",x=(xe=o.blockQueues)==null?void 0:xe[0],g=o.tunerCandidates??[],q=o.tunerEvaluatedCandidates??[],$=o.tunerActiveInterventions??[],M=[...q].sort((C,z)=>z.utilityScore-C.utilityScore||z.confidence-C.confidence),X=[...g].sort((C,z)=>z.ucbScore-C.ucbScore),I=o.tunerUtilityDelta??0,J=o.uclampPolicyMin??o.uclampMin,ae=o.uclampPolicyMax??o.uclampMax,F=O.thermalTone==="guard"?"md3-badge md3-badge-error":O.thermalTone==="elevated"?"md3-badge md3-badge-warning":"md3-badge md3-badge-success",re=O.pressureTone==="critical"?"md3-badge md3-badge-warning":O.pressureTone==="contended"?"md3-badge md3-badge-secondary":"md3-badge md3-badge-success";return r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col lg:flex-row lg:items-start justify-between gap-6 pb-6",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-2.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("span",{className:"md3-badge md3-badge-primary",children:c?o.engineActive?"NATIVE RUNTIME ACTIVE":"NATIVE STANDBY":`${j.label.toUpperCase()} (SIM)`}),r.jsxs("span",{className:F,children:["THERMAL: ",O.thermalTone.toUpperCase()]}),r.jsxs("span",{className:re,children:["PSI: ",O.pressureTone.toUpperCase()]}),r.jsxs("span",{className:"md3-badge md3-badge-neutral font-mono",children:["RISK: ",o.riskClass??j.risk]})]}),r.jsx("h2",{className:"text-xl sm:text-2xl font-bold tracking-tight leading-snug",children:c?o.modeDescription:j.description}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[O.headline," Every kernel mutation is gated by capability discovery, write-ahead CFJ4 journaling, synchronous readback verification, and empirical before/after outcome scoring."]}),O.activeDomains.length>0&&r.jsxs("div",{className:"flex flex-wrap items-center gap-1.5 pt-1",children:[r.jsx("span",{className:"text-[11px] font-semibold mr-1",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Active Domains:"}),O.activeDomains.map(C=>r.jsx("span",{className:"md3-badge md3-badge-secondary font-mono",children:C},C))]})]}),r.jsxs("div",{className:"md3-card-filled p-4 sm:p-5 shrink-0 lg:w-72 space-y-3",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("span",{className:"text-xs font-bold uppercase tracking-wider",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Equilibrium Index"}),r.jsxs("span",{className:"font-mono text-2xl font-bold tabular-nums",style:{color:"var(--md-sys-color-primary)"},children:[O.equilibriumScore,"%"]})]}),r.jsx("div",{className:"md3-progress-track",children:r.jsx("div",{className:"md3-progress-bar",style:{width:`${O.equilibriumScore}%`}})}),r.jsxs("div",{className:"pt-2 space-y-1 text-xs",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("div",{className:"font-semibold",children:"OEM Thermal → Pressure → Load → Balanced"}),r.jsxs("div",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Verdict: ",o.tunerVerdict??o.lastOutcome??j.outcome," · Ownership:"," ",o.ownershipIntact!==!1?"Intact":"Contended"]})]})]})]}),!c&&d&&r.jsxs("div",{className:"md3-card-filled px-4 py-3.5 flex flex-col sm:flex-row sm:items-center justify-between gap-3",children:[r.jsxs("div",{className:"flex items-center gap-2.5 text-xs",children:[r.jsx(Ng,{className:"w-4 h-4 shrink-0",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("div",{children:[r.jsx("span",{className:"font-bold",children:"Interactive Preview Workload Simulator"}),r.jsxs("span",{className:"hidden md:inline",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","· Inject synthetic pressure spikes to observe M3 transaction stages and bandit decisions"]})]})]}),r.jsxs("div",{className:"flex flex-wrap items-center gap-1.5",children:[r.jsxs("button",{type:"button",id:"btn-sim-cpu",onClick:()=>d("cpu"),className:"md3-chip",children:[r.jsx(Aa,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"CPU Burst"})]}),r.jsxs("button",{type:"button",id:"btn-sim-memory",onClick:()=>d("memory"),className:"md3-chip",children:[r.jsx(ra,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Memory Stall"})]}),r.jsxs("button",{type:"button",id:"btn-sim-io",onClick:()=>d("io_trial"),className:"md3-chip",children:[r.jsx(Nl,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Block I/O Trial"})]}),r.jsxs("button",{type:"button",id:"btn-sim-step",onClick:()=>d(void 0),className:"md3-btn-tonal !min-h-[2rem] !px-3 !text-xs",children:[r.jsx(ap,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Step Tick"})]})]})]}),r.jsxs("div",{className:"space-y-3 pb-6",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("span",{className:"text-xs font-bold flex items-center gap-1.5",children:[r.jsx($h,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Transaction Lifecycle Pipeline",c?"":" (sim)"]})]}),r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs font-mono",children:[o.tunerWorkload&&r.jsxs("span",{className:"md3-badge md3-badge-neutral",children:["Workload: ",o.tunerWorkload]}),o.tunerObjective&&r.jsxs("span",{className:"md3-badge md3-badge-neutral",children:["Objective: ",o.tunerObjective]}),r.jsxs("span",{className:"md3-badge md3-badge-primary",children:["Stage: ",o.tunerStage??R]})]})]}),r.jsx("div",{className:"grid grid-cols-2 sm:grid-cols-5 gap-2",children:Jc.map((C,z)=>{const P=Jc.findIndex(Me=>Me.key===R),ye=C.key===R,Re=z<P;return r.jsxs("div",{className:"px-3.5 py-2.5 rounded-2xl text-xs flex items-center justify-between transition-all",style:{backgroundColor:ye?"var(--md-sys-color-primary-container)":Re?"var(--md-sys-color-surface-container-high)":"var(--md-sys-color-surface-container)",color:ye?"var(--md-sys-color-on-primary-container)":Re?"var(--md-sys-color-on-surface)":"var(--md-sys-color-on-surface-variant)",fontWeight:ye?700:600},children:[r.jsxs("span",{className:"truncate",children:[z+1,". ",C.label]}),ye&&r.jsx("span",{className:"h-2 w-2 rounded-full shrink-0",style:{backgroundColor:"var(--md-sys-color-primary)"}})]},C.key)})})]}),r.jsxs("div",{className:"grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4",children:[r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Foreground UClamp · Readback"}),r.jsx(qg,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"text-2xl font-bold font-mono tabular-nums",children:[o.uclampSupported?o.uclampMin:"—",r.jsxs("span",{className:"text-xs font-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","/ ",o.uclampSupported?o.uclampMax:"—"]})]})]}),r.jsxs("div",{className:"mt-3 grid grid-cols-2 gap-2 pt-3 text-[11px]",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Policy base"}),r.jsxs("div",{className:"mt-0.5 font-mono font-bold",children:[J," / ",ae]})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"ABI encoding"}),r.jsx("div",{className:"mt-0.5 font-mono font-bold",children:o.uclampEncoding??"percentage"})]})]}),r.jsx("div",{className:"mt-2 truncate text-[10px] font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},title:o.uclampTargetPath??"",children:o.uclampSupported?o.uclampTargetPath||"Verified foreground cgroup":"Foreground cgroup capability unavailable"})]}),r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["SoC Thermal Composite",c?"":" (sim)"]}),r.jsx(ns,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"text-2xl font-bold font-mono tabular-nums",children:[L,r.jsxs("span",{className:"text-xs font-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","°C"]})]})]}),r.jsx("div",{className:"my-2.5",children:r.jsx("div",{className:"md3-progress-track",children:r.jsx("div",{className:"md3-progress-bar",style:{width:`${Math.max(8,Math.min(100,N>0?(N-28)/28*100:12))}%`}})})}),r.jsx("div",{className:"text-[11px] leading-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:c?`Source: ${o.thermalSource??"OEM policy & sensor fusion"}`:`Trip gate ${m.toFixed(1)} °C · CPU peak ${(o.cpuPeakCelsius??N).toFixed(1)} °C`})]}),r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["PSI Resource Pressure",c?"":" (sim)"]}),r.jsx(as,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-2 font-mono tabular-nums text-xs my-1",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CPU"}),r.jsx("div",{className:"text-sm font-bold",children:o.cpuPressureValid===!1?"N/A":`${o.cpuPressurePct.toFixed(1)}%`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"MEM"}),r.jsx("div",{className:"text-sm font-bold",children:o.memoryPressureValid===!1?"N/A":`${o.memoryPressurePct.toFixed(1)}%`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"I/O"}),r.jsx("div",{className:"text-sm font-bold",children:o.ioPressureValid===!1?"N/A":`${o.ioPressurePct.toFixed(1)}%`})]})]}),r.jsxs("div",{className:"text-[11px] mt-2 leading-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Linux ",r.jsx("code",{className:"font-mono",children:"/proc/pressure"})," avg10 window"]})]}),r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["CFJ4 Durable Journal",c?"":" (sim)"]}),r.jsx(On,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"text-lg font-bold",children:o.baselineStatus==="protected"?"Journal Armed":"Baseline Restored"})]}),r.jsx("div",{className:"text-[11px] mt-2 leading-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:o.baselineNodesSnapshotted>0?`${o.baselineNodesSnapshotted} owned kernel nodes verified`:"Write-ahead & readback verified"})]})]}),r.jsxs("div",{className:"md3-card-filled p-5 space-y-4",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-3 pb-3.5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-1 min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs",children:[r.jsx(gg,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{className:"font-bold text-sm",children:["Autonomous Contract-Gated Tuning Evaluator",c?"":" (sim)"]}),r.jsxs("span",{className:"md3-badge md3-badge-primary font-mono",children:["Intent: ",o.tunerIntent??"HoldEquilibrium"]})]}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:o.tunerRationale??"Evaluates workload-specific candidates only after the kernel contract, access path, valid range, and baseline have been verified."})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-4 shrink-0 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Empirical ΔU"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:I>=0?`+${I.toFixed(2)}`:I.toFixed(2)})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Retained"}),r.jsxs("div",{className:"font-bold text-sm mt-0.5",children:[o.tunerImproved??0," / ",o.tunerTrials??0]})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Rolled Back"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:o.tunerRolledBack??0})]})]})]}),$.length>0&&r.jsxs("div",{className:"space-y-2",children:[r.jsxs("div",{className:"text-xs font-bold",children:["Active Co-Executing Interventions (",$.length,")"]}),r.jsx("div",{className:"grid grid-cols-1 md:grid-cols-2 gap-2.5",children:$.map(C=>r.jsxs("div",{className:"md3-card-elevated p-3.5 text-xs space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2",children:[r.jsx("span",{className:"font-mono font-bold truncate",children:C.candidateId}),r.jsx("span",{className:"md3-badge md3-badge-primary font-mono",children:C.stage})]}),r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:C.candidateLabel}),r.jsxs("div",{className:"flex flex-wrap items-center gap-3 font-mono text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["Domain: ",C.domain]}),r.jsxs("span",{children:["Verdict: ",C.verdict]}),r.jsxs("span",{children:["Window: ",C.observationTick,"/",C.observationWindow]})]})]},`${C.domain}-${C.candidateId}`))})]}),r.jsxs("div",{className:"grid grid-cols-1 lg:grid-cols-2 gap-4",children:[r.jsxs("div",{className:"md3-card-elevated p-4 space-y-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs",children:[r.jsxs("span",{className:"font-bold flex items-center gap-1.5",children:[r.jsx(Nl,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Block Queue Surface",c?"":" (sim)"," (",x?`/sys/block/${x.device}/queue`:"Awaiting probe",")"]})]}),r.jsxs("span",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["OEM: ",(x==null?void 0:x.baselineScheduler)??"N/A"," → Active:"," ",r.jsx("strong",{style:{color:"var(--md-sys-color-primary)"},children:(x==null?void 0:x.activeScheduler)??"N/A"})]})]}),r.jsxs("div",{className:"grid grid-cols-3 sm:grid-cols-6 gap-2 pt-1 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"read_expire"}),r.jsx("div",{className:"font-bold",children:x&&x.readExpireMs>=0?`${x.readExpireMs}ms`:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"writes_starved"}),r.jsx("div",{className:"font-bold",children:x&&x.writesStarved>=0?x.writesStarved:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"fifo_batch"}),r.jsx("div",{className:"font-bold",children:x&&x.fifoBatch>=0?x.fifoBatch:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"read_ahead"}),r.jsx("div",{className:"font-bold",children:x&&x.readAheadKb>=0?`${x.readAheadKb}KB`:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"nr_requests"}),r.jsx("div",{className:"font-bold",children:x&&x.nrRequests>=0?x.nrRequests:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"rq_affinity"}),r.jsx("div",{className:"font-bold",children:x&&x.rqAffinity>=0?x.rqAffinity:"N/A"})]})]})]}),r.jsxs("div",{className:"md3-card-elevated p-4 space-y-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs",children:[r.jsx("span",{className:"font-bold",children:"Global Adaptive Candidate Ranking"}),r.jsxs("span",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Active: ",o.tunerCandidateId??"none"]})]}),r.jsx("div",{className:"max-h-60 space-y-2 overflow-y-auto pr-1",children:M.length>0?M.map((C,z)=>{const P=g.find(Re=>Re.id===C.id),ye=C.selected||C.id===o.tunerCandidateId;return r.jsxs("div",{className:"rounded-xl p-3 text-xs space-y-1.5",style:{backgroundColor:ye?"var(--md-sys-color-primary-container)":"var(--md-sys-color-surface-container)",color:ye?"var(--md-sys-color-on-primary-container)":"var(--md-sys-color-on-surface)"},children:[r.jsxs("div",{className:"flex items-start justify-between gap-3",children:[r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-1.5 text-[10px] font-mono",children:[r.jsxs("span",{children:["#",z+1]}),r.jsxs("span",{children:["· ",C.domain]}),ye&&r.jsx("span",{className:"md3-badge md3-badge-primary",children:"ACTIVE"}),!C.eligible&&r.jsx("span",{className:"md3-badge md3-badge-warning",children:"INELIGIBLE"})]}),r.jsx("div",{className:"mt-0.5 break-all font-bold",children:C.id})]}),r.jsxs("div",{className:"shrink-0 text-right font-mono tabular-nums",children:[r.jsx("div",{className:"text-sm font-bold",children:C.utilityScore.toFixed(1)}),r.jsx("div",{className:"text-[9px] opacity-75",children:"utility"})]})]}),r.jsxs("div",{className:"flex flex-wrap gap-x-3 gap-y-1 font-mono text-[10px] opacity-80",children:[r.jsxs("span",{children:["UCB ",C.empiricalScore.toFixed(3)]}),r.jsxs("span",{children:["conf ",Math.round(C.confidence*100),"%"]}),P&&r.jsxs("span",{children:["retained ",P.successes]}),P&&r.jsxs("span",{children:["rollback ",P.rollbacks]})]})]},C.id)}):X.map((C,z)=>r.jsxs("div",{className:"flex items-center justify-between gap-3 rounded-xl p-2.5",style:{backgroundColor:"var(--md-sys-color-surface-container)"},children:[r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"text-[10px] font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["#",z+1," · ",C.domain]}),r.jsx("div",{className:"truncate text-xs font-bold",children:C.id}),r.jsxs("div",{className:"text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["retained ",C.successes," · rollback ",C.rollbacks," · trials ",C.trials]})]}),r.jsxs("div",{className:"shrink-0 text-right font-mono",children:[r.jsx("div",{className:"text-sm font-bold",children:C.ucbScore.toFixed(3)}),r.jsx("div",{className:"text-[9px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"UCB1"})]})]},C.id))})]})]})]}),r.jsxs("div",{className:"md3-card-filled p-4 sm:p-5 flex flex-col sm:flex-row sm:items-center justify-between gap-4",children:[r.jsxs("div",{className:"space-y-1 min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs",children:[r.jsxs("span",{className:"font-bold text-sm",children:["Neural Horizon Predictor (ONNX / Online RLS-MLP)",c?"":" (sim)"]}),r.jsx("span",{className:"md3-badge md3-badge-primary font-mono",children:o.aiBackend??"embedded-rls-mlp"})]}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Forecasts 10-second thermal trajectory via Online Recursive Least Squares + 6×8×2 Micro-MLP with NLMS output-head adaptation, pre-pacing capacity strictly within Safety Admission invariants."})]}),r.jsxs("div",{className:"grid grid-cols-4 gap-4 shrink-0 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"10s Forecast"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:(o.aiPredictedPressure10s??45).toFixed(1)})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Velocity"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:(o.aiPredictedVelocity??0)>=0?`+${(o.aiPredictedVelocity??0).toFixed(2)}/s`:`${(o.aiPredictedVelocity??0).toFixed(2)}/s`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Scale Hint"}),r.jsxs("div",{className:"font-bold text-sm mt-0.5",children:[Math.round((o.aiRecommendedScale??1)*100),"%"]})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Confidence"}),r.jsxs("div",{className:"font-bold text-sm mt-0.5",style:{color:"var(--md-sys-color-primary)"},children:[Math.round((o.aiConfidence??.85)*100),"%"]})]})]})]})]})},Jf=({nodesSnapshotted:o,nativeMode:c,baselineStatus:m})=>r.jsxs("section",{className:"md3-card-elevated space-y-5 p-6 sm:p-7","aria-label":"Baseline ownership and recovery",children:[r.jsx("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-3",children:r.jsxs("div",{className:"flex items-start gap-3.5",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(On,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0 flex-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h3",{className:"text-base font-bold tracking-tight",children:"Ownership Guard & Baseline Recovery"}),r.jsx("span",{className:c?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",children:c?"NATIVE CFJ4 JOURNAL":"PREVIEW MODEL"})]}),r.jsx("p",{className:"mt-1 text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CoreFlow snapshots factory and pre-mutation values into a write-ahead journal before any write, then restores only nodes whose inode and active value prove continued ownership."})]})]})}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Journal Format"}),r.jsx(ru,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"font-mono text-sm font-bold",children:"CFJ4 · Write-Ahead"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Inode + boot_id bound"})]}),r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Ownership State"}),r.jsx(ou,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"text-sm font-bold",children:m==="protected"?"Recovery Protected":"Clean · Baseline Intact"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"External OEM writes respected"})]}),r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Verified Snapshots"}),r.jsx(cu,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"font-mono text-sm font-bold tabular-nums",children:c?o:`${o} (sim)`}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Idempotent rollback ready"})]})]}),r.jsx("p",{className:"pt-3 text-[11px] leading-relaxed",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:"Manual kernel policy sliders are intentionally absent. When a safety rollback is needed, the native recovery lifecycle and module kill switch remain authoritative."})]}),Qg={SCREEN_OFF:"Screen Off / Deep Idle",BALANCED_SWEET_SPOT:"Balanced Sweet-Spot",ACTIVE_LOAD:"Active Load / Gaming",THERMAL_GUARD:"Thermal Guard",READ_ONLY:"Read-only status",DISCOVERY:"Capability discovery"},$f=o=>{if(!Number.isFinite(o)||o<=0)return"Unavailable";const c=["B","KB","MB","GB","TB"];let m=o,d=0;for(;m>=1024&&d<c.length-1;)m/=1024,d+=1;return m.toFixed(d>=3?2:0)+" "+c[d]},Pg=o=>{const c=Number(o);return Number.isFinite(c)&&c>=1e5?`${Math.round(c/1e3)} MHz`:o},Kg=({status:o,nativeMode:c,journalOutput:m,restoringBaseline:d,onViewJournal:j,onRestoreBaseline:O})=>{if(!c)return r.jsxs("section",{className:"md3-card-elevated space-y-3 p-6 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx(ra,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"font-bold text-base",children:"Direct Adaptive Runtime"}),r.jsx("span",{className:"md3-badge md3-badge-neutral",children:"NATIVE ONLY"})]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Live topology discovery, actuator ownership, and OEM baseline values are queried directly from ",r.jsx("code",{className:"font-mono",children:"coreflowd"}),". Browser preview does not synthesize kernel audit evidence."]})]});if(!o||!o.active)return r.jsxs("section",{className:"md3-card-elevated space-y-3 p-6 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx(Si,{className:"h-4 w-4",style:{color:"var(--md-sys-color-warning)"}}),r.jsx("h2",{className:"font-bold text-base",children:"Direct Adaptive Runtime"})]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:(o==null?void 0:o.mode)==="READ_ONLY"?"Daemon is stopped. The last OEM baseline remains available for recovery.":"Runtime discovery or durable baseline preparation is unavailable. Independent daemon status remains accessible."})]});const N=o.appliedValues.filter(x=>x.owned).length,L=o.appliedValues.filter(x=>x.modified).length,B=o.ownershipLosses>0||o.appliedValues.some(x=>!x.owned&&x.modified),R=o.journalFault?"md3-badge md3-badge-error":o.thermalGuard?"md3-badge md3-badge-warning":"md3-badge md3-badge-success";return r.jsxs("section",{className:"md3-card-elevated space-y-5 p-6 sm:p-7","aria-label":"Direct Adaptive runtime and actuator audit",children:[r.jsxs("div",{className:"flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(ra,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"font-bold text-base sm:text-lg",children:"Direct Adaptive · Production Composition Root"})]}),r.jsx("p",{className:"mt-1 text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Topology-derived Sweet-Spot policy and readback-verified sysfs ownership. Optional PSI/FrameTimeline evidence never blocks independently discovered domains."})]}),r.jsxs("span",{className:R,children:[o.journalFault?r.jsx(Si,{className:"h-3.5 w-3.5"}):o.thermalGuard?r.jsx(as,{className:"h-3.5 w-3.5"}):r.jsx(On,{className:"h-3.5 w-3.5"}),o.journalFault?"Journal fault · mutations halted":Qg[o.mode]??o.mode]})]}),r.jsxs("div",{className:"grid grid-cols-2 gap-3 sm:grid-cols-4",children:[r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(Aa,{className:"h-3.5 w-3.5"})," CPU utilization"]}),r.jsxs("div",{className:"mt-1 font-mono text-lg font-bold tabular-nums",children:[o.cpuUtilizationPct.toFixed(1),"%"]})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(Nl,{className:"h-3.5 w-3.5"})," RAM / ZRAM"]}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold",children:$f(o.ramBytes)}),r.jsxs("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["ZRAM ",$f(o.zramBytes)]})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(ag,{className:"h-3.5 w-3.5"})," Display"]}),r.jsx("div",{className:"mt-1 font-mono text-lg font-bold",children:o.refreshHz>0?`${o.refreshHz} Hz`:"Auto"}),r.jsxs("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Screen ",o.screenState==="OFF"?"off":"on / active"]})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(as,{className:"h-3.5 w-3.5"})," Actuator audit"]}),r.jsx("div",{className:"mt-1 font-mono text-lg font-bold",children:o.verifiedMutations}),r.jsxs("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[o.skippedNodes," skipped · ",o.ownershipLosses," ownership losses"]})]})]}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px] font-bold uppercase tracking-wide",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"GPU Discovered"}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-bold",children:o.gpuModel})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px] font-bold uppercase tracking-wide",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Storage Topology"}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-bold",children:o.storageType})]})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2",children:[r.jsx("h3",{className:"text-sm font-bold",children:"CPU Topology & Discovered OPP"}),r.jsxs("span",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[o.cpuClusters.length," clusters · ",o.discoveredNodes," nodes"]})]}),o.cpuClusters.length===0?r.jsx("p",{className:"md3-card-filled p-3.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CPUFreq OPP table unavailable; CPU actuation is skipped independently."}):r.jsx("div",{className:"grid grid-cols-1 gap-3 md:grid-cols-2",children:o.cpuClusters.map(x=>r.jsxs("div",{className:"md3-card-filled min-w-0 p-3.5",children:[r.jsx("div",{className:"text-xs font-bold",children:x.id}),r.jsx("div",{className:"mt-2 flex flex-wrap gap-1.5",children:x.opps.map(g=>r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:Pg(g)},`${x.id}-${g}`))})]},x.id))})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsxs("div",{className:"flex flex-col gap-1 sm:flex-row sm:items-end sm:justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Current Applied Values vs OEM Baseline"}),r.jsxs("span",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[L," modified · ",N,"/",o.appliedValues.length," owned"]})]}),o.appliedValues.length===0?r.jsx("p",{className:"md3-card-filled p-3.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No writable semantic control nodes were captured."}):r.jsx("div",{className:"space-y-2",children:o.appliedValues.slice(0,24).map(x=>r.jsxs("div",{className:"md3-card-filled space-y-2 p-3.5",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"break-all font-mono text-xs font-bold",children:x.semantic}),r.jsx("div",{className:"break-all text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:x.domain})]}),r.jsxs("span",{className:x.owned?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",children:[x.owned?r.jsx(iu,{className:"h-3 w-3"}):r.jsx(Si,{className:"h-3 w-3"}),x.owned?"Owned / readback OK":x.health]})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-2 text-[11px]",children:[r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Current"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-bold",children:x.current})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"OEM"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-bold",children:x.oemBaseline})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Policy target"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-bold",children:x.target})]})]})]},x.path))})]}),B&&r.jsx("div",{role:"status",className:"rounded-2xl px-4 py-3 text-xs",style:{backgroundColor:"var(--md-sys-color-warning-container)",color:"var(--md-sys-color-on-warning-container)"},children:"One or more nodes lost mutation ownership or readback agreement. Those actuators are isolated; unrelated domains continue."}),r.jsxs("div",{className:"flex flex-wrap gap-2.5",children:[r.jsxs("button",{type:"button",onClick:j,className:"md3-btn-tonal",children:[r.jsx(Kh,{className:"h-3.5 w-3.5"})," View Mutation Journal"]}),r.jsxs("button",{type:"button",onClick:O,disabled:d,className:"md3-btn-outlined",style:{borderColor:"var(--md-sys-color-error)",color:"var(--md-sys-color-error)"},children:[r.jsx(ip,{className:"h-3.5 w-3.5"}),d?"Restoring OEM baseline…":"Restore OEM Baseline & Stop"]})]}),m!==null&&r.jsxs("div",{className:"space-y-2",children:[r.jsx("div",{className:"text-xs font-bold",children:"Baseline Journal · Read-Only"}),r.jsx("pre",{className:"max-h-64 overflow-auto rounded-2xl p-4 text-[11px] font-mono leading-relaxed",style:{backgroundColor:"var(--md-sys-color-surface-container-highest)",color:"var(--md-sys-color-on-surface)"},children:m})]}),r.jsxs("div",{className:"text-[11px] leading-relaxed font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["OEM snapshot entries: ",o.baselineNodes," · thermal pressure: ",o.thermalPressure.toFixed(1)," · journal state: ",o.journalFault?"fault-latched":"healthy"]})]})},Xg=[{id:"ALL",label:"All Events"},{id:"BOOST",label:"Sustained Load"},{id:"THERMAL",label:"Thermal Gate"},{id:"BASELINE",label:"Journal & Recovery"},{id:"INFO",label:"Admission & Info"}],Wr=({logs:o,nativeMode:c=!1})=>{const[m,d]=Q.useState("ALL"),[j,O]=Q.useState(""),[N,L]=Q.useState(!1),B=Q.useRef(null);Q.useEffect(()=>()=>{B.current!==null&&window.clearTimeout(B.current)},[]);const R=Q.useMemo(()=>o.filter(q=>{const $=m==="ALL"||q.type===m,M=!j||q.message.toLowerCase().includes(j.toLowerCase())||q.tag.toLowerCase().includes(j.toLowerCase());return $&&M}),[o,m,j]),x=async()=>{const q=R.map($=>`[${$.timestamp}] [${$.type}] ${$.tag}: ${$.message}`).join(`
`);try{await navigator.clipboard.writeText(q),L(!0),B.current!==null&&window.clearTimeout(B.current),B.current=window.setTimeout(()=>{L(!1),B.current=null},2e3)}catch{}},g=q=>{switch(q){case"THERMAL":return"md3-badge md3-badge-warning";case"BASELINE":return"md3-badge md3-badge-success";case"BOOST":return"md3-badge md3-badge-primary";default:return"md3-badge md3-badge-neutral"}};return r.jsxs("section",{className:"md3-card-elevated p-6 sm:p-7 space-y-5","aria-label":"Observability and audit trail",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-center justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-start gap-3.5",children:[r.jsx("div",{className:"w-11 h-11 rounded-2xl flex items-center justify-center shrink-0",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(uu,{className:"w-5 h-5"})}),r.jsxs("div",{className:"space-y-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h3",{className:"font-bold text-base sm:text-lg",children:"Observability & Audit Trail"}),r.jsxs("span",{className:"md3-badge md3-badge-neutral font-mono",children:[R.length," events"]})]}),r.jsx("p",{className:"text-xs sm:text-sm",style:{color:"var(--md-sys-color-on-surface-variant)"},children:c?"Live daemon transaction audit stream from runtime log ring.":"Preview transaction trace showing admission, readback verification, and outcome learning."})]})]}),r.jsx("div",{className:"flex items-center gap-2 shrink-0",children:r.jsxs("button",{type:"button",onClick:()=>void x(),className:"md3-btn-outlined",children:[N?r.jsx(au,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}):r.jsx(lu,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:N?"Copied":"Copy Audit Log"})]})})]}),r.jsxs("div",{className:"flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-3",children:[r.jsx("div",{className:"flex flex-wrap items-center gap-1.5",children:Xg.map(q=>{const $=m===q.id;return r.jsx("button",{type:"button","aria-pressed":$,onClick:()=>d(q.id),className:"md3-chip "+($?"is-selected":""),children:q.label},q.id)})}),r.jsxs("div",{className:"relative",children:[r.jsx(lp,{className:"w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none",style:{color:"var(--md-sys-color-on-surface-variant)"}}),r.jsx("input",{type:"text","aria-label":"Filter audit trail",placeholder:"Filter audit trail...",value:j,onChange:q=>O(q.target.value),className:"w-full sm:w-60 pl-9 pr-4 py-2 text-xs rounded-full focus:outline-none",style:{backgroundColor:"var(--md-sys-color-surface-container)",border:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface)"}})]})]}),r.jsx("div",{role:"log","aria-label":"Audit trail",className:"md3-card-filled p-3.5 max-h-80 overflow-y-auto font-mono text-xs divide-y",style:{borderColor:"var(--md-sys-color-outline-variant)"},children:R.length===0?r.jsx("div",{className:"text-center py-8 font-sans text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No audit events match the current filter."}):R.map(q=>r.jsxs("div",{className:"grid grid-cols-1 sm:grid-cols-[78px_98px_124px_1fr] items-baseline gap-2 py-2.5 px-2.5 rounded-xl transition-colors",style:{borderColor:"var(--md-sys-color-outline-variant)"},children:[r.jsx("span",{className:"tabular-nums text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:q.timestamp}),r.jsx("span",{children:r.jsx("span",{className:g(q.type),children:q.type})}),r.jsx("span",{className:"font-semibold truncate",children:q.tag}),r.jsx("span",{className:"break-words leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:q.message})]},q.id))})]})},Fg=[{id:"ocean",label:"Ocean",swatch:"#0b57d0"},{id:"emerald",label:"Emerald",swatch:"#006c4c"},{id:"amber",label:"Amber",swatch:"#b27b00"}],Zg=({isDark:o,onToggleTheme:c,accent:m="ocean",onSelectAccent:d,modeLabel:j,equilibriumScore:O,nativeMode:N=!1,nativeConnected:L=!1,onOpenArchitectureStudio:B})=>{const R=N&&L?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",x=N?L?"DAEMON LINKED":"AWAITING PAYLOAD":"SIMULATION PREVIEW";return r.jsx("header",{className:"md3-top-app-bar sticky top-0 z-30",children:r.jsxs("div",{className:"mx-auto flex min-h-[4.25rem] max-w-7xl items-center justify-between gap-3 px-4 py-2.5 sm:px-6",children:[r.jsxs("div",{className:"flex min-w-0 flex-1 items-center gap-3.5",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl shadow-xs",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(Aa,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex min-w-0 flex-wrap items-center gap-2",children:[r.jsx("span",{className:"truncate text-sm font-bold tracking-tight sm:text-base",children:"CFAE System Framework"}),r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:"v4.0.0"}),r.jsxs("span",{className:R,children:[r.jsx("span",{className:"h-1.5 w-1.5 rounded-full",style:{backgroundColor:N&&L?"var(--md-sys-color-success)":"var(--md-sys-color-warning)"}}),x]})]}),r.jsxs("p",{className:"mt-0.5 truncate text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:j}),r.jsx("span",{children:" · Coordinated Mutation Safety & Autonomous Equilibrium"})]})]})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2",children:[typeof O=="number"&&r.jsxs("div",{className:"hidden md:inline-flex items-center gap-1.5 rounded-full px-3 py-1.5 text-xs font-semibold",style:{backgroundColor:"var(--md-sys-color-surface-container)",border:"1px solid var(--md-sys-color-outline-variant)"},title:"Composite equilibrium index derived from thermal headroom, PSI pressure, and ownership health",children:[r.jsx(On,{className:"h-3.5 w-3.5",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{className:"font-mono tabular-nums",children:[O,"%"]}),r.jsx("span",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Equilibrium"})]}),d&&r.jsxs("div",{className:"hidden sm:inline-flex items-center gap-1 rounded-full px-2 py-1",style:{backgroundColor:"var(--md-sys-color-surface-container)",border:"1px solid var(--md-sys-color-outline-variant)"},role:"group","aria-label":"Material You seed color palette",children:[r.jsx(cg,{className:"h-3.5 w-3.5 ml-1 mr-0.5",style:{color:"var(--md-sys-color-on-surface-variant)"}}),Fg.map(g=>{const q=m===g.id;return r.jsx("button",{type:"button",id:`btn-accent-${g.id}`,"aria-pressed":q,"aria-label":`${g.label} Material 3 palette`,title:`${g.label} Material 3 palette`,onClick:()=>d(g.id),className:"h-5 w-5 rounded-full flex items-center justify-center transition-transform",style:{backgroundColor:g.swatch,boxShadow:q?"0 0 0 2px var(--md-sys-color-surface), 0 0 0 4px var(--md-sys-color-primary)":"none"}},g.id)})]}),r.jsxs("button",{type:"button",id:"btn-open-architecture-studio",onClick:B,title:"Open Architecture Studio","aria-label":"Open Architecture Studio",className:"md3-btn-tonal",children:[r.jsx(su,{className:"h-4 w-4"}),r.jsx("span",{className:"hidden sm:inline",children:"Studio"})]}),r.jsx("button",{type:"button",id:"btn-toggle-theme",onClick:c,className:"md3-icon-btn","aria-label":"Toggle color theme",title:o?"Switch to light theme":"Switch to dark theme",children:o?r.jsx(jg,{className:"h-4 w-4"}):r.jsx(lg,{className:"h-4 w-4"})})]})]})})},Jg="# CFAE System Framework v3.1.0 — C++23 + Rust 2024 + Kotlin Equilibrium & Mutation Safety Blueprint\n\n## 1. Runtime Architecture & Language Implementation Stack\n\n- **ISO C++23 Core (`src/daemon/`, `src/kernel/`, `include/coreflow/`)**: Handles semantic sysfs/cgroup v2 discovery, `ResourcePermissionClass` classification (`ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, `IncompleteMetadata`), `CentralMutationCoordinator` domain arbitration, Linux `epoll`/`timerfd`/`signalfd` event loop, `O_NOFOLLOW` + `fstat` (`dev:ino`) object identity checks, durable write-ahead journal (`CFJ4`), persistent bandit state (`CFTS2`), readback verification, and Android service capability probing.\n- **Rust 2024 Decision Engine (`rust/src/`)**: Owns the memory-safe thermal PID controller, hysteresis state machine, PSI pressure arbitration, invalid-sample rejection, and slew-bounded capacity budget via `cxx` FFI.\n- **Kotlin 2.1 Android Integration (`android/src/main/kotlin/io/coreflow/`)**: Provides native Android 15 (API 35) Quick Settings TileService (`CoreFlowTileService`) and thermal broadcast integration (`CoreFlowThermalReceiver`).\n- **Material Design 3 WebUI (`webui/`, `module/webroot/`)**: Lightweight offline dashboard synchronized between `dist/` and `module/webroot/`.\n\n## 2. Coordinated Mutation Transactions (6-Layer Pipeline)\n\nRather than rigid single-parameter-only rules or blind bulk writes, the primary unit of control is the **Coordinated Mutation Transaction**: one explicit optimization objective, a dependency-verified change plan within a bounded risk budget, domain ownership arbitration via `CentralMutationCoordinator`, per-write readback verification, settling-window + noise-floor validation, and ownership-safe rollback.\n\n1. **Universal Capability Classifier**: Classifies every discovered kernel node into `ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, or `IncompleteMetadata`. Unknown or unsafe nodes are never mutated.\n2. **Workload & Bottleneck Reasoning Engine**: Combines 11-state workload classification, CPU/Memory/I/O PSI, thermal headroom, and temperature trend into an inspectable `OptimizationObjective`.\n3. **Active Thermal Stabilization & Hysteresis Recovery**: Switches objective to `ThermalStabilization` (`thermal.schedutil_damped_pacing`, `thermal.io_burst_pacing`) under non-emergency thermal load, and transitions through hysteresis-guarded `ThermalRecovery` when cooling.\n4. **Central Mutation Coordinator & Dependency Analyzer**: Enforces single-mutation-per-domain leases, priority preemption (`EmergencySafety > ThermalStabilization > PressureGuard > WorkloadOptimization > BackgroundEfficiency`), and `RequiredAnchor -> RequiredDependent` ordering.\n5. **Safety Admission Gate & `CFJ4` Journal**: Verifies pre-write invariants, commits the `CFJ4` write-ahead journal with `boot_id`/`boot_epoch` and `dev:ino` identity prior to the first write, applies changes in dependency order, and verifies every write via readback.\n6. **Outcome Evaluator & `CFTS2` Persistent Learning**: Evaluates post-settling impact against the domain noise floor (`IMPROVED`, `NEUTRAL`, `REGRESSED`, `INCONCLUSIVE`, `ABORTED`, `OWNERSHIP_LOST`, `RECOVERY_REQUIRED`, `REJECTED`), steps down `adaptive_scale` on rollback, enforces context-aware anti-repeat cooldowns, and persists learning atomically in `CFTS2` format guarded by a capability fingerprint.\n\n## 3. Cross-Device Thermal & Power Policy\n\nRaw CPU/GPU junction, battery, and skin temperatures are distinct diagnostics and never share an arbitrary universal cutoff in native mode. Linux thermal-zone values are converted from the documented millidegree ABI.\n\nProduction decisions prioritize the Android NDK thermal severity and headroom API alongside recognized compute, battery, and skin zones evaluated against their read-only OEM passive/hot/critical trip points and secondary `hwmon` sensors. Missing OEM thermal policy retains the stock baseline and reports unavailable.\n\n## 4. Predictive Controller & Slew Bounding\n\nRust consumes a dimensionless thermal-policy pressure index. Its PID controller includes a bounded integral and pressure velocity, bounding output to `0.4–1.0` with a maximum slew of 2% per elapsed second. CPU/GPU ceilings are quantized to exposed operating points while honoring existing OEM maximums.\n\n## 5. Ownership Guard & Idempotent Recovery\n\nFor every mutation, `BaselineStore` records the baseline value, written value, last verified actual value, object identity (`dev:ino`), and ownership state. Rollback restores baseline values only if object identity matches and no external Android/OEM process has taken over (`OWNERSHIP_LOST` retires ownership without overwriting external changes). Crash recovery on startup is strictly idempotent.\n\n## 6. Hardware Introspection & Preservation\n\n- **CPU/GPU Ceilings**: Discovered from `cpufreq` and `devfreq` frequency tables. Missing tables disable that domain.\n- **Battery Step-Charging**: Uses standard `constant_charge_current_max`, evaluates battery temperature exclusively while charging, and never raises an OEM current ceiling.\n- **Storage I/O Weighting & Relative Queue Scaling**: Uses documented cgroup v2 `io.weight` (`default N` format) and scales block queue tunables (`read_ahead_kb`, `nr_requests`, `iosched/*`) relative to captured OEM baselines.\n- **Truthful Vulkan Pass-Through**: `libVkLayer_coreflow.so` reports only physical GPU ICD capabilities (`safe_hardware`) without spoofing API versions or unsupported features.\n\n",$g=`use crate::bridge::ffi::{Decision, Parameters, Snapshot};

pub struct Controller {
    parameters: Parameters,
    integral: f64,
    previous_pressure: Option<f64>,
    scale: f64,
    thermal_guard: bool,
}

pub fn new_controller(parameters: Parameters) -> Box<Controller> {
    Box::new(Controller {
        parameters, integral: 0.0, previous_pressure: None,
        scale: 1.0, thermal_guard: false,
    })
}

impl Controller {
    pub fn reset(&mut self) {
        self.integral = 0.0;
        self.previous_pressure = None;
        self.scale = 1.0;
        self.thermal_guard = false;
    }

    pub fn step(&mut self, input: Snapshot) -> Decision {
        let p = &self.parameters;
        let finite = [p.kp, p.ki, p.kd, p.target, input.thermal_pressure,
            input.elapsed_seconds, input.trip, input.recover, input.headroom,
            input.cpu_threshold, input.memory_threshold, input.io_threshold]
            .iter().all(|x| x.is_finite())
            && (!input.cpu_pressure_valid || input.cpu_pressure.is_finite())
            && (!input.memory_pressure_valid || input.memory_pressure.is_finite())
            && (!input.io_pressure_valid || input.io_pressure.is_finite());
        let pressures_valid = [input.cpu_threshold, input.memory_threshold, input.io_threshold]
            .iter().all(|x| (0.0..=100.0).contains(x))
            && (!input.cpu_pressure_valid || (0.0..=100.0).contains(&input.cpu_pressure))
            && (!input.memory_pressure_valid || (0.0..=100.0).contains(&input.memory_pressure))
            && (!input.io_pressure_valid || (0.0..=100.0).contains(&input.io_pressure));
        if !finite || !pressures_valid || !(0.0..=130.0).contains(&input.thermal_pressure)
            || input.elapsed_seconds <= 0.0 || input.elapsed_seconds > 120.0
            || input.recover >= input.trip || input.headroom < 0.0
            || !(0.0..=130.0).contains(&p.target) || p.target > input.trip
            || [p.kp, p.ki, p.kd].iter().any(|x| !(0.0..=20.0).contains(x)) {
            self.reset();
            return Decision { valid: false, state: 2, capacity_scale: 1.0 };
        }
        let dt = input.elapsed_seconds.clamp(0.001, 60.0);
        self.thermal_guard = input.thermal_pressure >= input.trip
            || (self.thermal_guard && input.thermal_pressure > input.recover);
        let state = if self.thermal_guard { 3 }
            else if (input.memory_pressure_valid && input.memory_pressure >= input.memory_threshold)
                 || (input.io_pressure_valid && input.io_pressure >= input.io_threshold) { 2 }
            else if input.cpu_pressure_valid && input.cpu_pressure >= input.cpu_threshold
                 && input.thermal_pressure <= input.trip - input.headroom { 1 }
            else { 0 };

        let mut target_scale = 1.0;
        if input.pid_enabled {
            let error = p.target - input.thermal_pressure;
            // Do not bank a large positive integral while the device is cold.
            self.integral = (self.integral + error * dt).clamp(-50.0, 0.0);
            let velocity = self.previous_pressure
                .map_or(0.0, |previous| (input.thermal_pressure - previous) / dt);
            let output = p.kp * error + p.ki * self.integral - p.kd * velocity;
            // 0.2 is the emergency thermal floor; the slew limiter still bounds each step.
            target_scale = (1.0 + output / 50.0).clamp(0.2, 1.0);
        }
        self.previous_pressure = Some(input.thermal_pressure);
        if self.thermal_guard { target_scale = target_scale.min(0.875); }
        // At most 2 percentage points per elapsed second, never a fixed cadence jump.
        let delta = (0.02 * dt).min(0.1);
        self.scale += (target_scale - self.scale).clamp(-delta, delta);
        // Preserve the emergency thermal floor after rate-limited convergence.
        self.scale = self.scale.clamp(0.2, 1.0);
        Decision { valid: true, state, capacity_scale: self.scale }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn sample(temp: f64) -> Snapshot {
        Snapshot { thermal_pressure: temp, elapsed_seconds: 1.0, cpu_pressure: 30.0,
            memory_pressure: 0.0, io_pressure: 0.0, cpu_pressure_valid: true,
            memory_pressure_valid: true, io_pressure_valid: true, trip: 44.0, recover: 40.0,
            headroom: 3.0, cpu_threshold: 25.0, memory_threshold: 15.0,
            io_threshold: 15.0, pid_enabled: true }
    }
    fn controller() -> Box<Controller> {
        new_controller(Parameters { kp: 2.5, ki: 0.15, kd: 1.2, target: 42.5 })
    }
    #[test]
    fn thermal_ramp_is_bounded_and_hysteretic() {
        let mut c = controller();
        assert_eq!(c.step(sample(38.0)).state, 1);
        let hot = c.step(sample(46.0));
        assert_eq!(hot.state, 3);
        assert!((hot.capacity_scale - 0.98).abs() < 1e-9);
        assert_eq!(c.step(sample(41.0)).state, 3);
        assert_ne!(c.step(sample(40.0)).state, 3);
    }
    #[test]
    fn invalid_telemetry_never_becomes_a_boost() {
        let mut c = controller();
        assert!(!c.step(sample(f64::NAN)).valid);
        let mut s = sample(35.0); s.memory_pressure = -1.0;
        assert!(!c.step(s).valid);
        let mut s = sample(35.0); s.elapsed_seconds = 200.0;
        assert!(!c.step(s).valid);
    }
    #[test]
    fn unavailable_psi_is_not_interpreted_as_zero_or_as_a_cpu_boost() {
        let mut c = controller();
        let mut s = sample(38.0);
        s.cpu_pressure_valid = false;
        s.memory_pressure_valid = false;
        s.io_pressure_valid = false;
        s.cpu_pressure = 0.0;
        s.memory_pressure = 0.0;
        s.io_pressure = 0.0;
        s.cpu_pressure = f64::NAN;
        s.memory_pressure = f64::NAN;
        s.io_pressure = f64::NAN;
        assert_eq!(c.step(s).state, 0);

        let mut s = sample(38.0);
        s.cpu_pressure_valid = false;
        s.cpu_pressure = 0.0;
        assert_eq!(c.step(s).state, 0);
    }
    #[test]
    fn available_pressure_domains_remain_independently_actionable() {
        let mut c = controller();
        let mut s = sample(38.0);
        s.memory_pressure_valid = false;
        s.io_pressure_valid = false;
        s.memory_pressure = 0.0;
        s.io_pressure = 0.0;
        assert_eq!(c.step(s).state, 1);
    }
    #[test]
    fn severe_thermal_shock_can_reach_emergency_floor() {
        let mut c = controller();
        for _ in 0..8 {
            let mut s = sample(130.0);
            s.elapsed_seconds = 60.0;
            let decision = c.step(s);
            assert!(decision.valid);
            assert!((0.2..=1.0).contains(&decision.capacity_scale));
        }
        assert!((c.scale - 0.2).abs() < 1e-9);
    }

    #[test]
    fn pressure_wins_over_cpu_boost_and_scale_stays_bounded() {
        let mut c = controller();
        let mut s = sample(38.0); s.io_pressure = 20.0;
        assert_eq!(c.step(s).state, 2);
        for _ in 0..1000 {
            let d = c.step(sample(47.0));
            assert!((0.2..=1.0).contains(&d.capacity_scale));
        }
    }
}
`,Wg=`use crate::controller::{Controller, new_controller};

#[cxx::bridge(namespace = "coreflow::engine")]
pub mod ffi {
    struct Parameters {
        kp: f64,
        ki: f64,
        kd: f64,
        target: f64,
    }
    struct Snapshot {
        thermal_pressure: f64,
        elapsed_seconds: f64,
        cpu_pressure: f64,
        memory_pressure: f64,
        io_pressure: f64,
        cpu_pressure_valid: bool,
        memory_pressure_valid: bool,
        io_pressure_valid: bool,
        trip: f64,
        recover: f64,
        headroom: f64,
        cpu_threshold: f64,
        memory_threshold: f64,
        io_threshold: f64,
        pid_enabled: bool,
    }
    struct Decision {
        valid: bool,
        state: u8,
        capacity_scale: f64,
    }
    extern "Rust" {
        type Controller;
        fn new_controller(parameters: Parameters) -> Box<Controller>;
        fn step(self: &mut Controller, input: Snapshot) -> Decision;
        fn reset(self: &mut Controller);
    }
}
`,e0=`/**
 * @file autonomous_tuner.cpp
 * @brief Self-Directed Multi-Domain Transaction Planner, Central Mutation Coordinator & Empirical Outcome Evaluator
 * @license MIT
 */
#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/config.hpp"
#include "coreflow/decision_engine.hpp"
#include "coreflow/foreground_task_resolver.hpp"
#include "coreflow/frequency_controller.hpp"
#include "coreflow/kernel_node_inventory.hpp"
#include "coreflow/transactional_frequency_actuator.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/worker_pool.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;

namespace {

std::string read_first_line(const std::string& path) {
    const auto res = KernelIo::read_first_line(path);
    return res.ok() ? res.value : std::string{};
}

bool parse_u64(const std::string& text, uint64_t& out) {
    return KernelIo::parse_u64_strict(DaemonConfig::trim(text), out);
}

std::vector<std::string> enumerate_attribute_names(const fs::path& directory,
                                                   std::size_t maximum = 128U) {
    std::vector<std::string> names;
    std::error_code ec;
    fs::directory_iterator it(directory, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && names.size() < maximum; it.increment(ec)) {
        std::error_code status_error;
        const auto status = it->symlink_status(status_error);
        if (status_error || fs::is_symlink(status) || !fs::is_regular_file(status)) continue;
        names.push_back(it->path().filename().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool is_physical_block_device(std::string_view name) {
    if (name.empty()) return false;
    // Exclude virtual block devices: loop*, ram*, zram*, dm-*, md*, nbd*, ublk*, z*
    if (name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
        name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0 ||
        name.rfind("md", 0) == 0 || name.rfind("nbd", 0) == 0 ||
        name.rfind("ublkb", 0) == 0 || name.rfind("zd", 0) == 0) {
        return false;
    }
    return name.rfind("sd", 0) == 0 ||
           name.rfind("mmcblk", 0) == 0 ||
           name.rfind("nvme", 0) == 0;
}

bool is_virtual_block_device(std::string_view name) {
    return name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0 ||
           name.rfind("zram", 0) == 0 || name.rfind("dm-", 0) == 0 ||
           name.rfind("md", 0) == 0 || name.rfind("nbd", 0) == 0 ||
           name.rfind("ublkb", 0) == 0 || name.rfind("zd", 0) == 0;
}

void parse_scheduler_line(
    const std::string& line,
    std::string& active_out,
    std::vector<std::string>& available_out) {
    active_out.clear();
    available_out.clear();
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        if (token.size() > 2 && token.front() == '[' && token.back() == ']') {
            const std::string clean = token.substr(1, token.size() - 2);
            active_out = clean;
            available_out.push_back(clean);
        } else {
            available_out.push_back(token);
        }
    }
    if (active_out.empty() && available_out.size() == 1) {
        active_out = available_out.front();
    }
}

TunerVerdict verdict_from_string(std::string_view v) noexcept {
    if (v == "IMPROVED") return TunerVerdict::Improved;
    if (v == "NEUTRAL") return TunerVerdict::Neutral;
    if (v == "REGRESSED") return TunerVerdict::Regressed;
    if (v == "ABORTED") return TunerVerdict::Aborted;
    if (v == "OWNERSHIP_LOST") return TunerVerdict::OwnershipLost;
    if (v == "INCONCLUSIVE") return TunerVerdict::Inconclusive;
    if (v == "RECOVERY_REQUIRED") return TunerVerdict::RecoveryRequired;
    if (v == "REJECTED") return TunerVerdict::Rejected;
    if (v == "PENDING") return TunerVerdict::Pending;
    return TunerVerdict::Hold;
}

std::string resolve_state_dir() {
    const char* test = std::getenv("COREFLOW_TEST_STATE_DIR");
    return (test && *test) ? std::string(test) : "/data/adb/coreflow";
}

// Open every directory component relative to the already verified parent. This
// rejects symlinked ancestors as well as a symlink at the final state directory.
UniqueFd open_state_directory(std::string_view path, bool create) noexcept {
    if (path.empty() || path.front() != '/') return {};
    UniqueFd current(::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (!current.valid()) return {};
    size_t pos = 1;
    while (pos < path.size()) {
        while (pos < path.size() && path[pos] == '/') ++pos;
        if (pos >= path.size()) break;
        const size_t end = path.find('/', pos);
        const size_t count = (end == std::string_view::npos ? path.size() : end) - pos;
        const std::string component(path.substr(pos, count));
        pos = (end == std::string_view::npos) ? path.size() : end + 1;
        if (component.empty() || component == ".") continue;
        if (component == "..") return {};
        int next = ::openat(current.get(), component.c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (next < 0 && errno == ENOENT && create) {
            if (::mkdirat(current.get(), component.c_str(), 0700) != 0 && errno != EEXIST) return {};
            next = ::openat(current.get(), component.c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        }
        if (next < 0) return {};
        struct stat st{};
        if (::fstat(next, &st) != 0 || !S_ISDIR(st.st_mode)) {
            (void)::close(next);
            return {};
        }
        current.reset(next);
    }
    return current;
}

bool write_all(int fd, std::string_view data) noexcept {
    return KernelIo::write_all_fd(fd, data);
}

bool read_bounded_regular_file(int fd, std::string& output, size_t max_bytes) {
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        static_cast<uint64_t>(st.st_size) > max_bytes) return false;
    output.clear();
    output.reserve(static_cast<size_t>(st.st_size));
    char buffer[4096];
    for (;;) {
        const ssize_t n = ::read(fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) return false;
        if (n == 0) return true;
        const size_t count = static_cast<size_t>(n);
        if (output.size() > max_bytes - count) return false;
        output.append(buffer, count);
    }
}

void remove_persisted_tuner_state() noexcept {
    UniqueFd dir = open_state_directory(resolve_state_dir(), false);
    if (!dir.valid()) return;
    if (::unlinkat(dir.get(), "tuner_stats.state", 0) == 0) (void)::fsync(dir.get());
}

bool check_zram_swap_active(const std::string& proc_root, const std::string& sys_root) {
    const fs::path swaps_path = fs::path(proc_root) / "swaps";
    const auto read_res = KernelIo::read_bounded_file(swaps_path.string(), 64U * 1024U);
    if (read_res.ok()) {
        return read_res.value.find("zram") != std::string::npos;
    }
    if (read_res.status != KernelIoStatus::NotFound) {
        return false;
    }
    std::error_code ec;
    return fs::exists(fs::path(sys_root) / "block/zram0", ec) && !ec;
}

std::string sanitize_tsv_field(std::string_view input) {
    if (input.empty()) return "-";
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        if (ch == '\\t' || ch == '\\n' || ch == '\\r' || ch == ' ') {
            out.push_back('_');
        } else {
            out.push_back(ch);
        }
    }
    return out.empty() ? "-" : out;
}


uint64_t tuner_state_checksum(std::string_view data) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : data) {
        hash ^= static_cast<uint64_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

void record_ownership_loss(CandidateStats* stats) {
    if (stats == nullptr) return;

    // Escalate quarantine after repeated OEM/external takeovers: 8, 16, 32, then
    // 64 monitor ticks (bounded to avoid permanent suppression from stale state).
    stats->ownership_loss_count = std::min<uint32_t>(stats->ownership_loss_count, 15U) + 1U;
    const uint32_t exponent = std::min<uint32_t>(stats->ownership_loss_count - 1U, 3U);
    const uint32_t backoff_ticks = std::min<uint32_t>(64U, 8U << exponent);
    stats->ownership_quarantine_ticks = std::max(stats->ownership_quarantine_ticks, backoff_ticks);
    stats->cooldown_ticks = std::max(stats->cooldown_ticks, backoff_ticks);
    stats->last_rollback_reason =
        "External ownership lost; candidate quarantined for " + std::to_string(backoff_ticks) + " ticks";
}

} // namespace

// ============================================================================
// CentralMutationCoordinator Implementation
// ============================================================================

// AutonomousTuner Implementation
// ============================================================================

AutonomousTuner::AutonomousTuner(CentralMutationCoordinator& coordinator,
                                 std::string sys_root,
                                 std::string proc_root,
                                 DeviceProfileHandle profile)
    : coordinator_(coordinator),
      controller_name_(CentralMutationCoordinator::make_controller_identity("AutonomousTuner")),
      sys_root_(std::move(sys_root)),
      proc_root_(std::move(proc_root)),
      device_profile_(std::move(profile)),
      worker_pool_(std::make_unique<BoundedWorkerPool>(2U, 32U, "coreflow-tuner")),
      decision_engine_(std::make_unique<GlobalDecisionEngine>()) {
    (void)coordinator_.register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id,
               std::string_view reason) noexcept {
            return handle_preemption(domain, transaction_id, reason);
        });
    init_candidates();
    discover(sys_root_, proc_root_);
}

AutonomousTuner::~AutonomousTuner() {
    {
        std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
        if (foreground_task_resolver_) {
            (void)foreground_task_resolver_->restore_all();
            foreground_task_resolver_ = nullptr;
        }
        if (!active_interventions_.empty() || !applied_steps_.empty() ||
            !unresolved_recoveries_.empty() ||
            (frequency_actuator_ &&
             (frequency_actuator_->mutation_active() || frequency_actuator_->recovery_required()))) {
            (void)rollback_active("Autonomous tuner teardown.");
        }
    }
    coordinator_.unregister_preemption_handler(controller_name_);
    if (worker_pool_) {
        worker_pool_->shutdown();
    }
}

void AutonomousTuner::set_device_profile(DeviceProfileHandle profile) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (!active_interventions_.empty() || !applied_steps_.empty() ||
        !unresolved_recoveries_.empty() || recovery_required_) {
        return;
    }
    device_profile_ = std::move(profile);
    discover(sys_root_, proc_root_);
}

void AutonomousTuner::configure_gpu_frequency_controller(FrequencyController* controller) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (!active_interventions_.empty() || recovery_required_) return;
    gpu_frequency_controller_ = controller;
}

void AutonomousTuner::configure_frequency_actuator(
    TransactionalFrequencyActuator* actuator,
    const TieredPolicyRouter* router,
    const PlatformContextSnapshot& platform) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (frequency_mutation_active_ ||
        (frequency_actuator_ != nullptr && frequency_actuator_->mutation_active())) return;
    frequency_actuator_ = actuator;
    tiered_policy_router_ = router;
    platform_context_ = platform;
}

bool AutonomousTuner::frequency_mutation_active() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    return frequency_mutation_active_ && frequency_actuator_ != nullptr &&
           frequency_actuator_->mutation_active();
}

bool AutonomousTuner::frequency_actuator_configured() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    return frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr;
}

std::optional<uint32_t> AutonomousTuner::select_frequency_policy_index() const noexcept {
    if (!device_profile_ || device_profile_->cpu_policies.empty()) return std::nullopt;
    uint64_t best_capacity = 0U;
    std::optional<uint32_t> selected;
    for (const auto& policy : device_profile_->cpu_policies) {
        if (policy.policy_path.empty() || !policy.related_cpus_valid ||
            policy.related_cpus.empty() || !policy.frequency_table_valid ||
            policy.cpuinfo_max_freq_khz == 0U) continue;
        const std::string name = fs::path(policy.policy_path).filename().string();
        if (name.rfind("policy", 0) != 0 || name.size() <= 6U) continue;
        uint32_t index = 0U;
        const std::string_view suffix(name.data() + 6U, name.size() - 6U);
        const auto parsed = std::from_chars(
            suffix.data(), suffix.data() + suffix.size(), index, 10);
        if (parsed.ec != std::errc{} || parsed.ptr != suffix.data() + suffix.size() ||
            index > 255U) continue;
        if (!selected.has_value() || policy.cpuinfo_max_freq_khz > best_capacity) {
            best_capacity = policy.cpuinfo_max_freq_khz;
            selected = index;
        }
    }
    return selected;
}

bool AutonomousTuner::handle_preemption(
    MutationDomain domain, std::string_view transaction_id, std::string_view reason) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    // 1. Retry any unresolved recovery record matching this exact domain & lease.
    for (size_t idx = 0; idx < unresolved_recoveries_.size(); ++idx) {
        if (unresolved_recoveries_[idx].domain == domain &&
            unresolved_recoveries_[idx].candidate_id == transaction_id) {
            const bool restored = retry_unresolved_recoveries();
            if (!restored) {
                last_verdict_ = "RECOVERY_REQUIRED";
                last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            }
            sync_primary_state_from_active();
            save_persisted_stats();
            return restored && std::none_of(
                unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
                [&](const FailedRecoveryRecord& rec) {
                    return rec.domain == domain && rec.candidate_id == transaction_id;
                });
        }
    }

    // 2. Locate exact active intervention owning (domain, transaction_id).
    for (size_t idx = 0; idx < active_interventions_.size(); ++idx) {
        auto& iv = active_interventions_[idx];
        if (iv.mutation_domain != domain || iv.candidate_id != transaction_id) {
            continue;
        }
        const bool had_applied_steps = !iv.applied_steps.empty() || iv.is_frequency_policy;
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
        if (auto* stats = find_candidate(iv.candidate_id)) ++stats->preempted_count;
        if (had_applied_steps) ++total_rolled_back_;
        const bool restored = rollback_intervention(iv, reason);
        if (restored) {
            active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
        } else {
            active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        }
        sync_primary_state_from_active();
        save_persisted_stats();
        return restored;
    }

    // 3. Handle preemption during an in-flight apply_transaction() before OBSERVING starts.
    if (current_candidate_id_ == transaction_id &&
        domain_for_candidate(current_candidate_id_) == domain) {
        const bool had_applied_steps = !applied_steps_.empty();
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
        if (auto* stats = find_candidate(current_candidate_id_)) ++stats->preempted_count;
        if (had_applied_steps) ++total_rolled_back_;
        bool restored = true;
        if (active_interventions_.empty()) {
            restored = rollback_active(reason);
        } else {
            restored = rollback_partial_steps(domain, transaction_id, applied_steps_, reason);
            sync_primary_state_from_active();
        }
        if (!restored) {
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        }
        save_persisted_stats();
        return restored;
    }

    return false;
}

void AutonomousTuner::init_candidates() {
    candidates_ = {
        CandidateStats{
            .id = "io.mq_deadline_low_latency",
            .domain = "io",
            .label = "mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "io.kyber_token_bucket",
            .domain = "io",
            .label = "kyber Token-Bucket Low-Latency I/O Scheduler",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "io.bfq_interactive_fairness",
            .domain = "io",
            .label = "bfq Proportional-Share Interactive I/O Scheduler",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "io.queue_readahead_pacing",
            .domain = "io",
            .label = "Adaptive Block Queue Read-Ahead & Request Depth (2.0x OEM baseline)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "vm.zram_latency_relief",
            .domain = "vm",
            .label = "VM Page-Cluster Zero-Latency & VFS Dentry Preservation",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "cpufreq.schedutil_interactive_response",
            .domain = "cpufreq",
            .label = "EAS Governor Rate-Limit Pacing (500us up / 10000us down)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "thermal.schedutil_damped_pacing",
            .domain = "thermal",
            .label = "Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "thermal.io_burst_pacing",
            .domain = "thermal",
            .label = "Thermal Stabilization Block Queue Burst Pacing (0.5x read-ahead / 0.75x depth)",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "cpu.frequency.tiered",
            .domain = "cpufreq",
            .label = "Tiered CPU Frequency OPP",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
        CandidateStats{
            .id = "gpu.kgsl.frame_deadline_relief",
            .domain = "gpu",
            .label = "Qualcomm KGSL Frame-Deadline Recovery",
            .mean_utility = 0.50,
            .mean_utility_delta = 0.0,
            .ucb_score = 0.50,
            .adaptive_scale = 1.0,
            .settling_ticks = 1,
            .trials = 0,
            .reward_samples = 0,
            .improved_count = 0,
            .successes = 0,
            .regressed_count = 0,
            .rollbacks = 0,
            .inconclusive_count = 0,
            .neutral_count = 0,
            .aborted_count = 0,
            .preempted_count = 0,
            .recovery_required_count = 0,
            .rejected_count = 0,
            .cooldown_ticks = 0,
            .consecutive_non_improved = 0,
            .ownership_loss_count = 0,
            .ownership_quarantine_ticks = 0,
            .last_workload = WorkloadClass::Idle,
            .last_thermal_bucket = 0,
            .last_before_summary = {},
            .last_after_summary = {},
            .last_rollback_reason = {},
        },
    };
    update_ucb_scores();
}

void AutonomousTuner::update_ucb_scores() {
    uint64_t non_gpu_reward_samples = 0U;
    uint64_t gpu_reward_samples = 0U;
    for (const auto& candidate : candidates_) {
        if (candidate.domain == "gpu") gpu_reward_samples += static_cast<uint64_t>(candidate.reward_samples);
        else non_gpu_reward_samples += static_cast<uint64_t>(candidate.reward_samples);
    }
    const double non_gpu_n = std::max(1.0, static_cast<double>(non_gpu_reward_samples));
    const double gpu_n = std::max(1.0, static_cast<double>(gpu_reward_samples));
    for (auto& candidate : candidates_) {
        candidate.successes = candidate.improved_count;
        candidate.rollbacks = candidate.regressed_count;
        const double pool_n = (candidate.domain == "gpu") ? gpu_n : non_gpu_n;
        const double explore = 0.25 * std::sqrt(std::log(pool_n + 1.0) / static_cast<double>(candidate.reward_samples + 1U));
        candidate.ucb_score = candidate.mean_utility + explore;
    }
}

void AutonomousTuner::reset() {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    // rollback_active releases the lease only after verified recovery. Do not
    // clear ownership here when rollback failed; recovery must remain fail-closed.
    if (!rollback_active("Autonomous tuner reset to verified OEM baseline.")) {
        return; // Preserve failed steps, recovery state, and the owning lease.
    }
    thermal_stabilization_active_ = false;
    thermal_recovery_ticks_ = 0;
    has_prev_thermal_ = false;
    noise_floor_ = 0.05;
    evaluation_noise_floors_.fill(0.05);
    current_evaluation_domain_ = EvaluationDomain::Unavailable;
    settling_ticks_remaining_ = 0;
    contention_backoff_ticks_ = 0;
    total_contention_retries_ = 0;
    applied_steps_.clear();
    active_interventions_.clear();
    unresolved_recoveries_.clear();
    last_evaluated_candidates_.clear();
    last_decision_plan_rationale_.clear();
    total_trials_ = 0;
    total_reward_samples_ = 0;
    total_improved_ = 0;
    init_candidates();
    total_rolled_back_ = 0;
    total_inconclusive_ = 0;
    total_rejected_ = 0;
    last_verdict_ = "HOLD";
    last_verdict_enum_ = TunerVerdict::Hold;
    last_utility_delta_ = 0.0;
    idle_streak_ = 0;
    remove_persisted_tuner_state();
}

CandidateStats* AutonomousTuner::find_candidate(std::string_view id) {
    for (auto& c : candidates_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

const CandidateStats* AutonomousTuner::find_candidate(std::string_view id) const {
    for (const auto& c : candidates_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

MutationDomain AutonomousTuner::domain_for_candidate(std::string_view candidate_id) const noexcept {
    if (candidate_id.rfind("io.", 0) == 0 || candidate_id == "thermal.io_burst_pacing") {
        return MutationDomain::BlockIo;
    }
    if (candidate_id.rfind("vm.", 0) == 0) {
        return MutationDomain::VmMemory;
    }
    if (candidate_id.rfind("gpu.", 0) == 0) return MutationDomain::GpuDevfreq;
    if (candidate_id.rfind("cpufreq.", 0) == 0 ||
        candidate_id == "cpu.frequency.tiered" ||
        candidate_id == "thermal.schedutil_damped_pacing") {
        return MutationDomain::CpuPolicy;
    }
    return MutationDomain::ThermalCore;
}

ControllerPriority AutonomousTuner::priority_for_candidate(std::string_view candidate_id) const noexcept {
    if (candidate_id.rfind("thermal.", 0) == 0) {
        return ControllerPriority::ThermalStabilization;
    }
    if (candidate_id.rfind("vm.", 0) == 0 || candidate_id.rfind("io.", 0) == 0) {
        return ControllerPriority::PressureGuard;
    }
    return ControllerPriority::WorkloadOptimization;
}

bool AutonomousTuner::is_candidate_eligible(std::string_view id) const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    const auto* c = find_candidate(id);
    if (c == nullptr || c->ownership_quarantine_ticks > 0 || c->cooldown_ticks > 0) return false;
    if (id == "vm.zram_latency_relief" && !zram_active_) return false;
    if (!candidate_contract_eligible(id)) return false;
    if ((id == "cpufreq.schedutil_interactive_response" || id == "thermal.schedutil_damped_pacing") &&
        cpufreq_governor_dirs_.empty()) {
        return false;
    }
    if ((id.rfind("io.", 0) == 0 || id == "thermal.io_burst_pacing") && block_queues_.empty()) {
        return false;
    }
    const MutationDomain dom = domain_for_candidate(id);
    const ControllerPriority prio = priority_for_candidate(id);
    if (coordinator_.is_domain_locked_by_other(dom, controller_name_, prio)) {
        return false;
    }
    return true;
}

bool AutonomousTuner::is_candidate_eligible_for_context(
    std::string_view id,
    WorkloadClass workload,
    uint8_t thermal_bucket) const noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    const auto* c = find_candidate(id);
    if (c == nullptr || c->ownership_quarantine_ticks > 0) return false;
    const auto regime_index = static_cast<std::size_t>(current_regime_);
    if (regime_index < c->regime_cooldown_ticks.size() &&
        c->regime_cooldown_ticks[regime_index] > 0U) return false;
    if (id == "vm.zram_latency_relief" && !zram_active_) return false;
    if (!candidate_contract_eligible(id)) return false;
    if ((id == "cpufreq.schedutil_interactive_response" || id == "thermal.schedutil_damped_pacing") &&
        cpufreq_governor_dirs_.empty()) {
        return false;
    }
    if ((id.rfind("io.", 0) == 0 || id == "thermal.io_burst_pacing") && block_queues_.empty()) {
        return false;
    }
    const MutationDomain dom = domain_for_candidate(id);
    const ControllerPriority prio = priority_for_candidate(id);
    if (coordinator_.is_domain_locked_by_other(dom, controller_name_, prio)) {
        return false;
    }
    if (c->cooldown_ticks > 0) {
        // Context-aware anti-repeat: keep candidate on cooldown in the same or adjacent context;
        // only allow early re-evaluation if cooldown is nearly expired (<= 2 ticks) AND both
        // workload class and thermal regime shifted significantly (>= 2 thermal buckets).
        const int bucket_diff = std::abs(static_cast<int>(thermal_bucket) - static_cast<int>(c->last_thermal_bucket));
        if (c->cooldown_ticks > 2 || workload == c->last_workload || bucket_diff < 2) {
            return false;
        }
    }
    return true;
}

uint8_t AutonomousTuner::compute_thermal_bucket(const TunerTelemetryInput& telemetry) noexcept {
    if (telemetry.thermal_emergency || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0)) return 4;
    if (telemetry.thermal_guard_active || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 65.0)) return 3;
    if (!telemetry.thermal_pressure_valid) return 0;
    if (telemetry.thermal_pressure >= 52.0) return 2;
    if (telemetry.thermal_pressure >= 42.0) return 1;
    return 0;
}

WorkloadClass AutonomousTuner::classify_workload(const TunerTelemetryInput& telemetry) noexcept {
    const bool rapid_temp_rise =
        (telemetry.temperature_velocity_valid && telemetry.temperature_velocity_celsius_per_sec >= 0.8) ||
        (telemetry.thermal_trend_celsius >= 0.8);
    if (telemetry.thermal_emergency || telemetry.thermal_guard_active ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 65.0) ||
        (telemetry.thermal_headroom >= 0.0 && telemetry.thermal_headroom <= 0.20) ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 56.0 && rapid_temp_rise)) {
        return WorkloadClass::ThermalConstrained;
    }
    if (telemetry.gpu_busy_valid && telemetry.frame_deadline_valid &&
        telemetry.frame_deadline_count >= 2U && telemetry.frame_deadline_misses > 0U &&
        telemetry.frame_deadline_misses <= telemetry.frame_deadline_count &&
        std::isfinite(telemetry.gpu_busy_pct) && telemetry.gpu_busy_pct >= 70.0 &&
        std::isfinite(telemetry.frame_deadline_miss_pct) &&
        telemetry.frame_deadline_miss_pct >= 0.5) {
        return WorkloadClass::GpuBound;
    }
    if (telemetry.memory_psi_valid && telemetry.io_psi_valid && telemetry.memory_psi >= 3.5 && telemetry.io_psi >= 3.0) {
        return WorkloadClass::HeavyMultitasking;
    }
    if (telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0) {
        return WorkloadClass::MemoryPressure;
    }
    if (telemetry.io_psi_valid && telemetry.cpu_psi_valid && telemetry.io_psi >= 5.0 && telemetry.cpu_psi >= 10.0) {
        return WorkloadClass::HeavyMultitasking;
    }
    if (telemetry.io_psi_valid && telemetry.io_psi >= 2.2) {
        return WorkloadClass::StorageIoPressure;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 16.0 && telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 50.0) {
        return WorkloadClass::Gaming;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 10.0) {
        return WorkloadClass::CpuBound;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 6.0) {
        return WorkloadClass::ScrollingAndAnimation;
    }
    if ((telemetry.cpu_psi_valid && telemetry.cpu_psi >= 2.5) || (telemetry.io_psi_valid && telemetry.io_psi >= 1.0)) {
        return WorkloadClass::GeneralInteraction;
    }
    if (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 1.2) {
        return WorkloadClass::BackgroundWorkload;
    }
    return WorkloadClass::Idle;
}

DynamicWorkloadRegime AutonomousTuner::classify_regime(
    const TunerTelemetryInput& telemetry, WorkloadClass workload) noexcept {
    const bool thermal_constrained = telemetry.thermal_emergency || telemetry.thermal_guard_active ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 65.0) ||
        (telemetry.thermal_headroom >= 0.0 && telemetry.thermal_headroom <= 0.20);
    if (thermal_constrained || workload == WorkloadClass::Gaming ||
        workload == WorkloadClass::CpuBound || workload == WorkloadClass::GpuBound ||
        workload == WorkloadClass::ThermalConstrained)
        return DynamicWorkloadRegime::HeavySustained;

    const double io = telemetry.io_psi_valid ? telemetry.io_psi : 0.0;
    const double memory = telemetry.memory_psi_valid ? telemetry.memory_psi : 0.0;
    if (io >= 4.0 || memory >= 3.0 || workload == WorkloadClass::StorageIoPressure)
        return DynamicWorkloadRegime::BurstLoading;
    if (io >= 1.5 || memory >= 1.5 || workload == WorkloadClass::MemoryPressure ||
        workload == WorkloadClass::HeavyMultitasking)
        return DynamicWorkloadRegime::ModerateLoad;
    if (workload == WorkloadClass::GeneralInteraction ||
        workload == WorkloadClass::ScrollingAndAnimation ||
        (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 2.5))
        return DynamicWorkloadRegime::Interactive;
    if (workload == WorkloadClass::BackgroundWorkload ||
        (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 0.5))
        return DynamicWorkloadRegime::LightLoad;
    return DynamicWorkloadRegime::IdleDeep;
}

std::string_view AutonomousTuner::regime_name(DynamicWorkloadRegime regime) noexcept {
    switch (regime) {
        case DynamicWorkloadRegime::IdleDeep: return "IDLE_DEEP";
        case DynamicWorkloadRegime::LightLoad: return "LIGHT_LOAD";
        case DynamicWorkloadRegime::Interactive: return "INTERACTIVE";
        case DynamicWorkloadRegime::ModerateLoad: return "MODERATE_LOAD";
        case DynamicWorkloadRegime::BurstLoading: return "BURST_LOADING";
        case DynamicWorkloadRegime::HeavySustained: return "HEAVY_SUSTAINED";
    }
    return "IDLE_DEEP";
}

BottleneckKind AutonomousTuner::diagnose_bottleneck(
    const TunerTelemetryInput& telemetry,
    WorkloadClass workload) noexcept {
    if (telemetry.thermal_emergency || (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0)) {
        return BottleneckKind::ThermalEmergency;
    }
    if (workload == WorkloadClass::ThermalConstrained) {
        return BottleneckKind::ThermalHeadroomDepletion;
    }
    if (telemetry.io_psi_valid && telemetry.cpu_psi_valid && telemetry.io_psi >= 4.5 && telemetry.cpu_psi >= 9.0) {
        return BottleneckKind::ConcurrentCpuIoContention;
    }
    const double io_sev = (telemetry.io_psi_valid && telemetry.io_psi >= 2.2)
        ? std::max(0.05, std::clamp((telemetry.io_psi - 0.8) / 14.0, 0.0, 1.0))
        : -1.0;
    const double mem_sev = (telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0)
        ? std::max(0.05, std::clamp((telemetry.memory_psi - 0.8) / 14.0, 0.0, 1.0))
        : -1.0;
    const double cpu_sev = (telemetry.cpu_psi_valid && telemetry.cpu_psi >= 8.0)
        ? std::max(0.05, std::clamp((telemetry.cpu_psi - 2.0) / 26.0, 0.0, 1.0))
        : -1.0;
    if (mem_sev >= 0.0 && mem_sev >= io_sev && mem_sev >= cpu_sev) {
        return BottleneckKind::MemorySwapThrashing;
    }
    if (cpu_sev >= 0.0 && cpu_sev >= io_sev) {
        return BottleneckKind::CpuSchedulingLatency;
    }
    if (io_sev >= 0.0) {
        return BottleneckKind::IoQueueContention;
    }
    return BottleneckKind::None;
}

OptimizationObjective AutonomousTuner::select_objective(
    const TunerTelemetryInput& telemetry,
    WorkloadClass workload,
    BottleneckKind bottleneck,
    bool in_thermal_recovery) noexcept {
    if (bottleneck == BottleneckKind::ThermalEmergency) {
        return OptimizationObjective::EmergencyMitigation;
    }
    if (bottleneck == BottleneckKind::ThermalHeadroomDepletion || workload == WorkloadClass::ThermalConstrained) {
        return OptimizationObjective::ThermalStabilization;
    }
    if (in_thermal_recovery) {
        return OptimizationObjective::ThermalRecovery;
    }
    switch (bottleneck) {
        case BottleneckKind::ConcurrentCpuIoContention:
            return OptimizationObjective::ThroughputBalance;
        case BottleneckKind::IoQueueContention:
            return OptimizationObjective::IoLatencyRelief;
        case BottleneckKind::MemorySwapThrashing:
            return OptimizationObjective::MemoryRelief;
        case BottleneckKind::CpuSchedulingLatency:
            return (workload == WorkloadClass::Gaming)
                       ? OptimizationObjective::ThroughputBalance
                       : OptimizationObjective::InteractiveLatency;
        case BottleneckKind::None:
        case BottleneckKind::ThermalHeadroomDepletion:
        case BottleneckKind::ThermalEmergency:
            break;
    }
    (void)telemetry;
    return OptimizationObjective::HoldBaseline;
}

std::string AutonomousTuner::compute_fingerprint() const {
    std::string fp = "v2|zram=" + std::string(zram_active_ ? "1" : "0");
    if (device_profile_) fp += "|cddp=" + device_profile_->fingerprint;
    for (const auto& q : block_queues_) {
        fp += "|dev=" + q.device_name + ":";
        for (const auto& s : q.available_schedulers) {
            fp += s + ",";
        }
    }
    for (const auto& g : cpufreq_governor_dirs_) {
        fp += "|gov=" + fs::path(g).filename().string();
    }
    return fp;
}

void AutonomousTuner::save_persisted_stats() const {
    std::ostringstream out;
    out << "CFTS8\\t" << compute_fingerprint() << "\\t"
        << total_trials_ << "\\t" << total_improved_ << "\\t" << total_rolled_back_ << "\\t"
        << total_reward_samples_ << "\\t" << total_inconclusive_ << "\\t" << total_rejected_ << "\\t" << noise_floor_;
    for (const double floor : evaluation_noise_floors_) out << "\\t" << floor;
    out << "\\n";
    for (const auto& c : candidates_) {
        out << c.id << "\\t" << c.mean_utility << "\\t" << c.mean_utility_delta << "\\t"
            << c.trials << "\\t" << c.reward_samples << "\\t" << c.improved_count << "\\t"
            << c.regressed_count << "\\t" << c.neutral_count << "\\t" << c.aborted_count << "\\t"
            << c.preempted_count << "\\t" << c.recovery_required_count << "\\t" << c.cooldown_ticks << "\\t"
            << c.consecutive_non_improved << "\\t" << c.adaptive_scale << "\\t"
            << c.inconclusive_count << "\\t" << c.rejected_count << "\\t"
            << static_cast<uint32_t>(c.last_workload) << "\\t"
            << static_cast<uint32_t>(c.last_thermal_bucket) << "\\t"
            << c.ownership_loss_count << "\\t" << c.ownership_quarantine_ticks << "\\t"
            << static_cast<uint32_t>(c.last_regime);
        for (std::size_t i = 0; i < c.regime_reward_samples.size(); ++i) {
            out << "\\t" << c.regime_reward_samples[i]
                << "\\t" << c.regime_mean_utility[i]
                << "\\t" << c.regime_regressions[i]
                << "\\t" << c.regime_cooldown_ticks[i];
        }
        out << "\\t" << sanitize_tsv_field(c.last_rollback_reason) << "\\n";
    }
    std::string payload = out.str();
    if (!out.good() || payload.size() > 256U * 1024U - 64U) return;
    payload += "CFTS8_END\\t" + std::to_string(tuner_state_checksum(payload)) + "\\n";

    UniqueFd dir = open_state_directory(resolve_state_dir(), true);
    if (!dir.valid()) return;

    static std::atomic<uint64_t> temp_nonce{0};
    std::string temp_name;
    UniqueFd temp_fd;
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        temp_name = "tuner_stats.state.tmp." + std::to_string(::getpid()) + "." +
                    std::to_string(temp_nonce.fetch_add(1, std::memory_order_relaxed));
        const int fd = ::openat(dir.get(), temp_name.c_str(),
                                O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (fd >= 0) { temp_fd.reset(fd); break; }
        if (errno != EEXIST) return;
    }
    if (!temp_fd.valid()) return;

    struct stat temp_stat{};
    if (::fstat(temp_fd.get(), &temp_stat) != 0 || !S_ISREG(temp_stat.st_mode) ||
        ::fchmod(temp_fd.get(), 0600) != 0 || !write_all(temp_fd.get(), payload) ||
        ::fsync(temp_fd.get()) != 0) {
        (void)::unlinkat(dir.get(), temp_name.c_str(), 0);
        return;
    }
    if (::renameat(dir.get(), temp_name.c_str(), dir.get(), "tuner_stats.state") != 0) {
        (void)::unlinkat(dir.get(), temp_name.c_str(), 0);
        return;
    }
    (void)::fsync(dir.get());
}

void AutonomousTuner::load_persisted_stats() {
    UniqueFd dir = open_state_directory(resolve_state_dir(), false);
    if (!dir.valid()) return;
    UniqueFd state_fd(::openat(dir.get(), "tuner_stats.state",
                               O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!state_fd.valid()) return;

    std::string payload;
    if (!read_bounded_regular_file(state_fd.get(), payload, 256U * 1024U)) return;

    bool has_checksum = false;
    std::string checksum_footer_magic;
    const size_t footer_pos8 = payload.rfind("CFTS8_END\\t");
    const size_t footer_pos7 = payload.rfind("CFTS7_END\\t");
    const size_t footer_pos6 = payload.rfind("CFTS6_END\\t");
    if ((footer_pos8 != std::string::npos) + (footer_pos7 != std::string::npos) +
        (footer_pos6 != std::string::npos) > 1) return;
    const size_t footer_pos = footer_pos8 != std::string::npos ? footer_pos8 :
        (footer_pos7 != std::string::npos ? footer_pos7 : footer_pos6);
    if (footer_pos != std::string::npos) {
        if (footer_pos > 0 && payload[footer_pos - 1] != '\\n') return;
        const std::string footer = payload.substr(footer_pos);
        std::istringstream footer_stream(footer);
        std::string extra;
        uint64_t stored_checksum = 0;
        if (!(footer_stream >> checksum_footer_magic >> stored_checksum) ||
            (checksum_footer_magic != "CFTS8_END" && checksum_footer_magic != "CFTS7_END" &&
             checksum_footer_magic != "CFTS6_END") ||
            (footer_stream >> extra)) return;
        payload.resize(footer_pos);
        if (tuner_state_checksum(payload) != stored_checksum) return;
        has_checksum = true;
    }

    std::istringstream in(payload);
    std::string header;
    if (!std::getline(in, header)) return;
    std::istringstream hs(header);
    std::string magic, fp, extra;
    uint32_t trials = 0, improved = 0, rolled_back = 0;
    if (!(hs >> magic >> fp >> trials >> improved >> rolled_back)) return;
    if ((magic == "CFTS8" && (!has_checksum || checksum_footer_magic != "CFTS8_END")) ||
        (magic == "CFTS7" && (!has_checksum || checksum_footer_magic != "CFTS7_END")) ||
        (magic == "CFTS6" && (!has_checksum || checksum_footer_magic != "CFTS6_END")) ||
        (magic != "CFTS8" && magic != "CFTS7" && magic != "CFTS6" && has_checksum)) return;

    const std::string current_fp = compute_fingerprint();
    uint32_t persisted_reward_samples = 0;
    uint32_t loaded_inconclusive = 0;
    uint32_t loaded_rejected = 0;
    double loaded_noise_floor = 0.05;
    std::array<double, 4> loaded_domain_floors{{0.05, 0.05, 0.05, 0.05}};

    if (magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4") {
        if (fp != current_fp ||
            !(hs >> persisted_reward_samples >> loaded_inconclusive >> loaded_rejected >> loaded_noise_floor) ||
            persisted_reward_samples > trials ||
            !std::isfinite(loaded_noise_floor) || loaded_noise_floor < 0.02 || loaded_noise_floor > 0.45) return;
        if (magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5") {
            for (auto& floor : loaded_domain_floors) {
                if (!(hs >> floor) || !std::isfinite(floor) || floor < 0.02 || floor > 0.45) return;
            }
        } else {
            loaded_domain_floors.fill(loaded_noise_floor);
        }
        if (hs >> extra) return;
    } else if (magic == "CFTS3" || magic == "CFTS2") {
        if (fp != current_fp) return;
        double legacy_floor = 0.05;
        if (hs >> loaded_inconclusive >> loaded_rejected >> legacy_floor) {
            if (!std::isfinite(legacy_floor) || legacy_floor < 0.02 || legacy_floor > 0.45) return;
            loaded_noise_floor = legacy_floor;
            loaded_domain_floors.fill(legacy_floor);
        } else {
            hs.clear();
        }
        if (hs >> extra) return;
    } else if (magic == "CFTS1") {
        std::string v1_fp = current_fp;
        if (v1_fp.rfind("v2|", 0) == 0) v1_fp.replace(0, 3, "v1|");
        if (fp != v1_fp || (hs >> extra)) return;
    } else {
        return;
    }

    struct ParsedCandidate {
        std::string id;
        double mean_utility{0.5};
        double mean_utility_delta{0.0};
        double adaptive_scale{1.0};
        uint32_t trials{0}, reward_samples{0}, improved{0}, regressed{0}, neutral{0};
        uint32_t aborted{0}, preempted{0}, recovery_required{0}, cooldown{0}, consecutive_non_improved{0};
        uint32_t inconclusive{0}, rejected{0}, workload{0}, thermal_bucket{0};
        uint32_t ownership_losses{0}, quarantine_ticks{0};
        uint32_t last_regime{0};
        std::array<double, 6> regime_means{};
        std::array<uint32_t, 6> regime_samples{};
        std::array<uint32_t, 6> regime_regressions{};
        std::array<uint32_t, 6> regime_cooldowns{};
        std::string rollback_reason;
    };
    std::vector<ParsedCandidate> parsed;
    std::unordered_set<std::string> seen_ids;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line.size() > 4096 || parsed.size() >= 256) return;
        std::istringstream ls(line);
        ParsedCandidate saved;
        if (!(ls >> saved.id >> saved.mean_utility >> saved.mean_utility_delta >> saved.trials)) return;
        if (saved.id.empty() || saved.id.size() > 128 || !seen_ids.insert(saved.id).second ||
            !std::isfinite(saved.mean_utility) || saved.mean_utility < 0.05 || saved.mean_utility > 2.50 ||
            !std::isfinite(saved.mean_utility_delta)) return;

        const bool current_schema = magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4";
        if (current_schema) {
            if (!(ls >> saved.reward_samples >> saved.improved >> saved.regressed >> saved.neutral >> saved.aborted)) return;
            if ((magic == "CFTS8" || magic == "CFTS7") && !(ls >> saved.preempted)) return;
            if (!(ls >> saved.recovery_required >> saved.cooldown >> saved.consecutive_non_improved)) return;
            // Legacy CFTS3 counters were retained during the R1 migration while
            // reward_samples intentionally started at zero. Do not reinterpret those
            // historical outcome totals as eligible modern reward observations.
            if (saved.reward_samples > saved.trials) return;
        } else {
            if (!(ls >> saved.improved >> saved.regressed >> saved.cooldown >> saved.consecutive_non_improved)) return;
            // Pre-R1 trial totals mixed attempts with outcomes, so their historical
            // utility estimate has no defensible sample denominator. Preserve the
            // counters for diagnostics, but restart learning from an uninformative
            // prior until new ownership-verified reward samples are collected.
            saved.mean_utility = 0.5;
            saved.mean_utility_delta = 0.0;
            saved.reward_samples = 0;
            saved.neutral = 0;
            saved.aborted = 0;
            saved.recovery_required = 0;
        }

        if (!(ls >> saved.adaptive_scale >> saved.inconclusive >> saved.rejected >>
              saved.workload >> saved.thermal_bucket)) return;
        if (!std::isfinite(saved.adaptive_scale)) return;
        saved.adaptive_scale = std::clamp(saved.adaptive_scale, 0.25, 1.0);
        saved.cooldown = std::min<uint32_t>(saved.cooldown, 64U);
        saved.consecutive_non_improved = std::min<uint32_t>(saved.consecutive_non_improved, 16U);
        saved.workload = std::min<uint32_t>(saved.workload, 10U);
        saved.thermal_bucket = std::min<uint32_t>(saved.thermal_bucket, 4U);

        if (magic == "CFTS8" || magic == "CFTS7" || magic == "CFTS6" || magic == "CFTS5" || magic == "CFTS4" || magic == "CFTS3") {
            if (!(ls >> saved.ownership_losses >> saved.quarantine_ticks)) return;
        }
        if (magic == "CFTS8") {
            if (!(ls >> saved.last_regime) || saved.last_regime >= 6U) return;
            for (std::size_t i = 0; i < saved.regime_samples.size(); ++i) {
                if (!(ls >> saved.regime_samples[i] >> saved.regime_means[i] >>
                      saved.regime_regressions[i] >> saved.regime_cooldowns[i])) return;
                if (saved.regime_samples[i] > saved.reward_samples ||
                    !std::isfinite(saved.regime_means[i]) ||
                    (saved.regime_samples[i] > 0U &&
                     (saved.regime_means[i] < 0.05 || saved.regime_means[i] > 2.50)) ||
                    (saved.regime_samples[i] == 0U && saved.regime_means[i] != 0.0) ||
                    saved.regime_regressions[i] > saved.trials ||
                    saved.regime_cooldowns[i] > 64U) return;
            }
        }
        if (!std::getline(ls, saved.rollback_reason)) return;
        if (!saved.rollback_reason.empty() && saved.rollback_reason.front() == '\\t')
            saved.rollback_reason.erase(saved.rollback_reason.begin());
        if (saved.rollback_reason.empty()) saved.rollback_reason = "-";
        if (saved.rollback_reason.size() > 512 ||
            saved.rollback_reason.find_first_of("\\t\\r\\n") != std::string::npos) return;
        saved.ownership_losses = std::min<uint32_t>(saved.ownership_losses, 16U);
        saved.quarantine_ticks = std::min<uint32_t>(saved.quarantine_ticks, 64U);
        std::string trailing;
        if (ls >> trailing) return;
        parsed.push_back(std::move(saved));
    }
    if (in.bad()) return;

    // Commit parsed state only after the complete file, checksum, header, and every
    // candidate record have passed validation. Learning-state corruption must never
    // partially overwrite in-memory controller state or BaselineStore recovery state.
    total_trials_ = trials;
    total_reward_samples_ = persisted_reward_samples;
    total_improved_ = improved;
    total_rolled_back_ = rolled_back;
    total_inconclusive_ = loaded_inconclusive;
    total_rejected_ = loaded_rejected;
    noise_floor_ = std::clamp(loaded_noise_floor, 0.02, 0.45);
    evaluation_noise_floors_ = loaded_domain_floors;

    for (const auto& saved : parsed) {
        if (auto* candidate = find_candidate(saved.id)) {
            candidate->mean_utility = saved.reward_samples > 0U
                ? saved.mean_utility : 0.50;
            candidate->mean_utility_delta = saved.mean_utility_delta;
            candidate->adaptive_scale = saved.adaptive_scale;
            candidate->trials = saved.trials;
            candidate->reward_samples = saved.reward_samples;
            candidate->improved_count = saved.improved;
            candidate->successes = saved.improved;
            candidate->regressed_count = saved.regressed;
            candidate->rollbacks = saved.regressed;
            candidate->neutral_count = saved.neutral;
            candidate->aborted_count = saved.aborted;
            candidate->preempted_count = saved.preempted;
            candidate->recovery_required_count = saved.recovery_required;
            candidate->cooldown_ticks = saved.cooldown;
            candidate->consecutive_non_improved = saved.consecutive_non_improved;
            candidate->inconclusive_count = saved.inconclusive;
            candidate->rejected_count = saved.rejected;
            candidate->last_workload = static_cast<WorkloadClass>(saved.workload);
            candidate->last_thermal_bucket = static_cast<uint8_t>(saved.thermal_bucket);
            if (magic == "CFTS8") {
                candidate->last_regime = static_cast<DynamicWorkloadRegime>(saved.last_regime);
                candidate->regime_reward_samples = saved.regime_samples;
                candidate->regime_mean_utility = saved.regime_means;
                candidate->regime_regressions = saved.regime_regressions;
                candidate->regime_cooldown_ticks = saved.regime_cooldowns;
            }
            candidate->ownership_loss_count = saved.ownership_losses;
            candidate->ownership_quarantine_ticks = saved.quarantine_ticks;
            candidate->last_rollback_reason = saved.rollback_reason == "-" ? "" : saved.rollback_reason;
        }
    }
    update_ucb_scores();
}

std::string AutonomousTuner::resolve_vm_node_path(std::string_view semantic_id) const {
    if (device_profile_) {
        const auto found = std::find_if(device_profile_->node_capabilities.begin(),
            device_profile_->node_capabilities.end(), [&](const NodeCapability& node) {
                return node.semantic_id == semantic_id && node.domain == HardwareDomain::Memory &&
                       node.source == ProfileSource::Procfs;
            });
        if (found != device_profile_->node_capabilities.end()) return found->path;
    }

    // Compatibility path for standalone/tests without a pinned CDDP profile:
    // enumerate the VM virtual table and match the standardized ABI name.
    const std::string leaf = semantic_id == "vm.page_cluster" ? "page-cluster" :
        semantic_id == "vm.vfs_cache_pressure" ? "vfs_cache_pressure" : std::string{};
    if (leaf.empty()) return {};
    const fs::path root = fs::path(proc_root_) / "sys/vm";
    std::error_code ec;
    for (fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (it->path().filename() == leaf) {
            std::error_code type_ec;
            const auto status = it->symlink_status(type_ec);
            if (!type_ec && !fs::is_symlink(status) && fs::is_regular_file(status))
                return it->path().string();
        }
    }
    return {};
}

bool AutonomousTuner::has_runtime_node(std::string_view path) const noexcept {
    return CandidateSelector::has_runtime_path(kernel_node_capabilities_, path);
}

bool AutonomousTuner::candidate_contract_eligible(std::string_view candidate_id) const noexcept {
    const bool storage_candidate = candidate_id.rfind("io.", 0) == 0 ||
        candidate_id == "thermal.io_burst_pacing";
    if (storage_candidate) {
        for (const auto& queue : block_queues_) {
            const std::string scheduler = queue.queue_dir + "/scheduler";
            const std::string read_ahead = queue.queue_dir + "/read_ahead_kb";
            const std::string requests = queue.queue_dir + "/nr_requests";
            if (candidate_id == "io.queue_readahead_pacing") {
                if (has_runtime_node(read_ahead)) return true;
                continue;
            }
            if (candidate_id == "thermal.io_burst_pacing") {
                if (has_runtime_node(read_ahead) && has_runtime_node(requests)) return true;
                continue;
            }
            if (has_runtime_node(scheduler) && has_runtime_node(read_ahead)) return true;
        }
        return false;
    }

    if (candidate_id == "vm.zram_latency_relief") {
        const auto page_cluster = resolve_vm_node_path("vm.page_cluster");
        const auto cache_pressure = resolve_vm_node_path("vm.vfs_cache_pressure");
        return !page_cluster.empty() && !cache_pressure.empty() &&
               has_runtime_node(page_cluster) && has_runtime_node(cache_pressure);
    }

    if (candidate_id == "cpufreq.schedutil_interactive_response" ||
        candidate_id == "thermal.schedutil_damped_pacing") {
        return std::any_of(cpufreq_governor_dirs_.begin(), cpufreq_governor_dirs_.end(),
            [&](const std::string& governor_dir) {
                return has_runtime_node(governor_dir + "/up_rate_limit_us") &&
                       has_runtime_node(governor_dir + "/down_rate_limit_us");
            });
    }
    if (candidate_id == "cpu.frequency.tiered") {
        return frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr &&
               select_frequency_policy_index().has_value();
    }
    if (candidate_id == "gpu.kgsl.frame_deadline_relief") {
        if (gpu_frequency_controller_ == nullptr || gpu_frequency_controller_->kgsl_count() == 0U ||
            !gpu_frequency_controller_->gpu_frequency_observation_ready() || !device_profile_)
            return false;
        return std::any_of(device_profile_->gpu_domains.begin(), device_profile_->gpu_domains.end(),
            [&](const GpuDomainProfile& domain) {
                if (domain.device_path.find("/class/kgsl/") == std::string::npos) return false;
                return has_runtime_node(domain.device_path + "/max_pwrlevel") &&
                       has_runtime_node(domain.device_path + "/min_pwrlevel");
            });
    }
    return true;
}

void AutonomousTuner::classify_discovered_resources() {
    discovered_resources_.clear();
    std::error_code ec;

    // 1. Classify all block devices (both physical and virtual/unsafe)
    const fs::path block_root = fs::path(sys_root_) / "class/block";
    for (fs::directory_iterator it(block_root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && discovered_resources_.size() < 64;
         it.increment(ec)) {
        const std::string dev_name = it->path().filename().string();
        const fs::path qdir = it->path() / "queue";
        const std::string sched_path = (qdir / "scheduler").string();
        if (!fs::exists(sched_path, ec) || ec) continue;

        if (is_virtual_block_device(dev_name)) {
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = sched_path,
                .domain = MutationDomain::BlockIo,
                .permission_class = ResourcePermissionClass::WritableButUnsafe,
                .unit = "token",
                .range_provenance = "virtual_block_queue",
                .baseline_value = read_first_line(sched_path),
                .reason = "Virtual block device (" + dev_name + ") excluded from scheduler mutation",
            });
            continue;
        }
        if (!is_physical_block_device(dev_name)) {
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = sched_path,
                .domain = MutationDomain::BlockIo,
                .permission_class = ResourcePermissionClass::UnknownSemantic,
                .unit = "unknown",
                .range_provenance = "unrecognized_block_device",
                .baseline_value = read_first_line(sched_path),
                .reason = "Unrecognized block device prefix; semantics unverified",
            });
            continue;
        }

        const bool writable = (::access(sched_path.c_str(), R_OK | W_OK) == 0);
        std::string active;
        std::vector<std::string> avail;
        parse_scheduler_line(read_first_line(sched_path), active, avail);
        discovered_resources_.push_back(DiscoveredResourceMetadata{
            .path = sched_path,
            .domain = MutationDomain::BlockIo,
            .permission_class = !writable ? ResourcePermissionClass::ReadOnly
                                : avail.empty() ? ResourcePermissionClass::IncompleteMetadata
                                : has_runtime_node(sched_path) ? ResourcePermissionClass::VerifiedConstraints
                                                              : ResourcePermissionClass::UnknownSemantic,
            .unit = "scheduler_token",
            .range_provenance = "sysfs:queue/scheduler",
            .baseline_value = active,
            .reason = !writable ? "Read-only scheduler node"
                : has_runtime_node(sched_path) ? "Verified scheduler ABI and immutable OEM baseline"
                                               : "Scheduler is observable; mutation contract/baseline not verified",
        });

        for (const auto& [leaf, unit_str, range_str] : {
                 std::tuple{"read_ahead_kb", "KiB", "[32..512] relative to OEM baseline"},
                 std::tuple{"nr_requests", "requests", "[32..512] relative to OEM baseline"},
                 std::tuple{"rq_affinity", "mode", "{0,1,2}"},
             }) {
            const std::string p = (qdir / leaf).string();
            if (!fs::exists(p, ec) || ec) continue;
            uint64_t parsed = 0;
            const std::string raw = read_first_line(p);
            const bool valid_num = parse_u64(raw, parsed);
            const bool leaf_w = (::access(p.c_str(), R_OK | W_OK) == 0);
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = p,
                .domain = MutationDomain::BlockIo,
                .permission_class = !valid_num ? ResourcePermissionClass::UnknownSemantic
                                    : !leaf_w  ? ResourcePermissionClass::ReadOnly
                                    : has_runtime_node(p) ? ResourcePermissionClass::VerifiedConstraints
                                                          : ResourcePermissionClass::UnknownSemantic,
                .unit = unit_str,
                .range_provenance = range_str,
                .baseline_value = raw,
                .reason = !valid_num ? "Non-numeric sysfs content"
                          : !leaf_w  ? "Read-only queue node"
                          : has_runtime_node(p) ? "Verified numeric queue ABI and immutable OEM baseline"
                                                : "Numeric queue attribute remains observation-only",
            });
        }
    }

    // 2. Classify CPUFreq policy & governor nodes.
    if (device_profile_) {
        for (const auto& cap : device_profile_->node_capabilities) {
            if (cap.domain != HardwareDomain::Cpu ||
                cap.semantic_id != "cpufreq.governor_tunable.unverified") continue;
            ResourcePermissionClass permission = ResourcePermissionClass::IncompleteMetadata;
            if (CandidateSelector::is_mutation_candidate(cap)) {
                permission = ResourcePermissionClass::VerifiedConstraints;
            } else if (cap.state == NodeCapabilityState::Writable) {
                permission = ResourcePermissionClass::WritableButUnsafe;
            } else if (cap.state == NodeCapabilityState::Readable) {
                permission = ResourcePermissionClass::ReadOnly;
            }
            discovered_resources_.push_back(DiscoveredResourceMetadata{
                .path = cap.path,
                .domain = MutationDomain::CpuPolicy,
                .permission_class = permission,
                .unit = cap.unit,
                .range_provenance = cap.constraints,
                .baseline_value = cap.current_value,
                .reason = cap.rejection_reason.empty() ? cap.evidence : cap.rejection_reason,
            });
        }
    } else {
    const fs::path cpufreq_root = fs::path(sys_root_) / "devices/system/cpu/cpufreq";
    std::error_code cpu_ec;
    for (fs::directory_iterator it(cpufreq_root, fs::directory_options::skip_permission_denied, cpu_ec), end;
         !cpu_ec && it != end && discovered_resources_.size() < 96;
         it.increment(cpu_ec)) {
        const std::string pol_name = it->path().filename().string();
        if (pol_name.rfind("policy", 0) != 0) continue;

        const std::string active_gov = DaemonConfig::trim(
            read_first_line((it->path() / "scaling_governor").string()));
        if (active_gov.empty() || active_gov == "." || active_gov == ".." ||
            active_gov.find('/') != std::string::npos || active_gov.find('\\0') != std::string::npos) continue;
        const fs::path governor_dir = it->path() / active_gov;
        std::error_code governor_ec;
        if (!fs::is_directory(governor_dir, governor_ec) || governor_ec) continue;
        const std::string up_path = (governor_dir / "up_rate_limit_us").string();
        const std::string down_path = (governor_dir / "down_rate_limit_us").string();
        const bool both_exist = fs::is_regular_file(up_path, governor_ec) && !governor_ec &&
                                fs::is_regular_file(down_path, governor_ec) && !governor_ec;
        const bool both_w = both_exist && (::access(up_path.c_str(), R_OK | W_OK) == 0) &&
                            (::access(down_path.c_str(), R_OK | W_OK) == 0);
        discovered_resources_.push_back(DiscoveredResourceMetadata{
            .path = up_path,
            .domain = MutationDomain::CpuPolicy,
            .permission_class = !both_exist ? ResourcePermissionClass::IncompleteMetadata :
                both_w ? ResourcePermissionClass::WritableButUnsafe : ResourcePermissionClass::ReadOnly,
            .unit = "microseconds",
            .range_provenance = "Active governor pair; ABI-specific limits require verified contract",
            .baseline_value = read_first_line(up_path),
            .reason = both_w ? "Active governor pair discovered; no policy value changed during discovery" :
                "Active governor pair incomplete or not writable",
        });
    }

    }
    // 3. Classify VM nodes
    for (const auto& [leaf, unit_str, range_str] : {
             std::tuple{"page-cluster", "log2_pages", "[0..3] requires active zram"},
             std::tuple{"vfs_cache_pressure", "percentage", "[50..150] requires active zram"},
         }) {
        const std::string semantic = std::string_view(leaf) == "page-cluster" ? "vm.page_cluster" : "vm.vfs_cache_pressure";
        const std::string p = resolve_vm_node_path(semantic);
        if (p.empty() || !fs::exists(p, ec) || ec) continue;
        uint64_t parsed = 0;
        const std::string raw = read_first_line(p);
        const bool valid_num = parse_u64(raw, parsed);
        const bool writable = (::access(p.c_str(), R_OK | W_OK) == 0);
        ResourcePermissionClass perm = has_runtime_node(p)
            ? ResourcePermissionClass::VerifiedConstraints : ResourcePermissionClass::UnknownSemantic;
        std::string reason = has_runtime_node(p)
            ? "Verified VM tunable ABI and immutable OEM baseline"
            : "VM node is observed; runtime contract/baseline not verified";
        if (!valid_num) {
            perm = ResourcePermissionClass::UnknownSemantic;
            reason = "Unparseable VM tunable value";
        } else if (!writable) {
            perm = ResourcePermissionClass::ReadOnly;
            reason = "Read-only procfs VM tunable";
        } else if (!zram_active_) {
            perm = ResourcePermissionClass::WritableButUnsafe;
            reason = "Writable VM node gated because zram swap is inactive";
        }
        discovered_resources_.push_back(DiscoveredResourceMetadata{
            .path = p,
            .domain = MutationDomain::VmMemory,
            .permission_class = perm,
            .unit = unit_str,
            .range_provenance = range_str,
            .baseline_value = raw,
            .reason = reason,
        });
    }
}

void AutonomousTuner::set_operational(bool enabled) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (!enabled && foreground_task_resolver_) {
        (void)foreground_task_resolver_->restore_all();
    }
    operational_ = enabled;
}

void AutonomousTuner::configure_foreground_task_resolver(
    ForegroundTaskResolver* resolver) noexcept {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    if (foreground_task_resolver_ == resolver) return;
    if (foreground_task_resolver_ && !foreground_task_resolver_->restore_all()) return;
    foreground_task_resolver_ = resolver;
}

void AutonomousTuner::discover(std::string_view sys_root, std::string_view proc_root) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    sys_root_ = std::string(sys_root);
    proc_root_ = std::string(proc_root);
    block_queues_.clear();
    cpufreq_governor_dirs_.clear();
    zram_active_ = check_zram_swap_active(proc_root_, sys_root_);

    const fs::path block_root = fs::path(sys_root_) / "class/block";
    std::error_code ec;
    for (fs::directory_iterator it(block_root, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && block_queues_.size() < 8;
         it.increment(ec)) {
        const std::string dev_name = it->path().filename().string();
        if (!is_physical_block_device(dev_name)) continue;

        const fs::path qdir = it->path() / "queue";
        const std::string sched_path = (qdir / "scheduler").string();
        if (::access(sched_path.c_str(), R_OK) != 0) continue;

        const std::string sched_raw = read_first_line(sched_path);
        if (sched_raw.empty()) continue;

        BlockQueueSurface surface{};
        surface.device_name = dev_name;
        surface.queue_dir = qdir.string();
        parse_scheduler_line(sched_raw, surface.active_scheduler, surface.available_schedulers);
        if (surface.active_scheduler.empty()) continue;
        surface.baseline_scheduler = surface.active_scheduler;
        surface.queue_tunables = enumerate_attribute_names(qdir);
        surface.iosched_tunables = enumerate_attribute_names(qdir / "iosched");
        surface.scheduler_generation = 1U;

        uint64_t val = 0;
        if (parse_u64(read_first_line((qdir / "read_ahead_kb").string()), val)) {
            surface.baseline_read_ahead_kb = val;
            surface.active_read_ahead_kb = val;
            surface.read_ahead_kb = val;
        }
        if (parse_u64(read_first_line((qdir / "nr_requests").string()), val)) {
            surface.baseline_nr_requests = val;
            surface.active_nr_requests = val;
            surface.nr_requests = val;
        }
        if (parse_u64(read_first_line((qdir / "rq_affinity").string()), val)) {
            surface.rq_affinity = val;
        }

        const fs::path iosched = qdir / "iosched";
        if (fs::is_directory(iosched, ec) && !ec && !surface.iosched_tunables.empty()) {
            surface.has_iosched_tunables = true;
            if (parse_u64(read_first_line((iosched / "read_expire").string()), val)) {
                surface.read_expire_ms = val;
            }
            if (parse_u64(read_first_line((iosched / "writes_starved").string()), val)) {
                surface.writes_starved = val;
            }
            if (parse_u64(read_first_line((iosched / "fifo_batch").string()), val)) {
                surface.fifo_batch = val;
            }
        }

        // Pre-capture baseline values in BaselineStore for safe restoration
        auto& store = BaselineStore::instance();
        (void)store.capture(sched_path);
        (void)store.capture((qdir / "read_ahead_kb").string());
        (void)store.capture((qdir / "nr_requests").string());
        (void)store.capture((qdir / "rq_affinity").string());
        if (surface.has_iosched_tunables) {
            (void)store.capture((iosched / "read_expire").string());
            (void)store.capture((iosched / "writes_starved").string());
            (void)store.capture((iosched / "fifo_batch").string());
        }

        block_queues_.push_back(std::move(surface));
    }

    // Sort deterministically by device name (e.g. sda before sdb)
    std::sort(block_queues_.begin(), block_queues_.end(), [](const auto& a, const auto& b) {
        return a.device_name < b.device_name;
    });

    auto& store = BaselineStore::instance();
    // Discover CPUFreq governor rate-limit pairs only from the injected profile.
    // No profile means legacy compatibility; an injected profile suppresses all
    // independent CPUFreq enumeration, even when it rejects every candidate.
    if (device_profile_) {
        const auto eligible_capability = [&](const std::string& path) {
            return std::any_of(device_profile_->node_capabilities.begin(),
                device_profile_->node_capabilities.end(), [&](const NodeCapability& cap) {
                    return cap.path == path && cap.mutation_eligible();
                });
        };
        for (const auto& policy : device_profile_->cpu_policies) {
            if (policy.governor.empty() || policy.governor == "." || policy.governor == ".." ||
                policy.governor.find('/') != std::string::npos ||
                policy.governor.find('\\0') != std::string::npos) continue;
            const fs::path policy_path(policy.policy_path);
            const fs::path governor_dir = policy_path / policy.governor;
            const std::string up = (governor_dir / "up_rate_limit_us").string();
            const std::string down = (governor_dir / "down_rate_limit_us").string();
            if (eligible_capability(up) && eligible_capability(down)) {
                (void)store.capture(up);
                (void)store.capture(down);
                cpufreq_governor_dirs_.push_back(governor_dir.string());
            }
        }
    } else {
    // Legacy compatibility discovery for non-CDDP callers.
    // Verify policyX/scaling_governor matches the governor directory if scaling_governor exists.
    const fs::path cpufreq_root = fs::path(sys_root_) / "devices/system/cpu/cpufreq";
    std::error_code cpu_ec;
    for (fs::directory_iterator it(cpufreq_root, fs::directory_options::skip_permission_denied, cpu_ec), end;
         !cpu_ec && it != end && cpufreq_governor_dirs_.size() < 8;
         it.increment(cpu_ec)) {
        const std::string pol_name = it->path().filename().string();
        if (pol_name.rfind("policy", 0) != 0) continue;
        const fs::path scaling_gov_path = it->path() / "scaling_governor";
        const std::string active_gov = DaemonConfig::trim(read_first_line(scaling_gov_path.string()));
        if (active_gov.empty() || active_gov == "." || active_gov == ".." ||
            active_gov.find('/') != std::string::npos || active_gov.find('\\0') != std::string::npos) continue;
        const fs::path governor_dir = it->path() / active_gov;
        const std::string up_path = (governor_dir / "up_rate_limit_us").string();
        const std::string down_path = (governor_dir / "down_rate_limit_us").string();
        if (::access(up_path.c_str(), R_OK | W_OK) == 0 &&
            ::access(down_path.c_str(), R_OK | W_OK) == 0) {
            (void)store.capture(up_path);
            (void)store.capture(down_path);
            cpufreq_governor_dirs_.push_back(governor_dir.string());
        }
    }
    }
    std::sort(cpufreq_governor_dirs_.begin(), cpufreq_governor_dirs_.end());

    // Capture VM baseline nodes if accessible
    const auto page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const auto cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    if (!page_cluster_path.empty()) (void)store.capture(page_cluster_path);
    if (!cache_pressure_path.empty()) (void)store.capture(cache_pressure_path);

    // Pin the semantic inventory used by every candidate admission and
    // transaction preflight.  A profile is authoritative when supplied;
    // standalone/test construction gets the same bounded scanner after OEM
    // baselines have been captured above.
    kernel_node_capabilities_.clear();
    if (device_profile_) {
        kernel_node_capabilities_ = device_profile_->node_capabilities;
    } else {
        KernelNodeInventory inventory(sys_root_, proc_root_);
        kernel_node_capabilities_ = inventory.scan(512U);
    }

    classify_discovered_resources();
    load_persisted_stats();
}

void AutonomousTuner::refresh_surface_readings() {
    zram_active_ = check_zram_swap_active(proc_root_, sys_root_);
    auto refresh_queue = [](BlockQueueSurface& q) {
        std::string active;
        std::vector<std::string> avail;
        parse_scheduler_line(read_first_line(q.queue_dir + "/scheduler"), active, avail);
        const bool parent_policy_changed = !active.empty() && active != q.active_scheduler;
        if (!active.empty()) q.active_scheduler = active;
        if (!avail.empty()) q.available_schedulers = std::move(avail);
        if (parent_policy_changed || q.scheduler_generation == 0U) {
            // Scheduler-specific ABI children are invalidated by a parent policy
            // transition. Re-enumerate the currently active iosched directory now.
            q.queue_tunables = enumerate_attribute_names(q.queue_dir);
            q.iosched_tunables = enumerate_attribute_names(fs::path(q.queue_dir) / "iosched");
            q.has_iosched_tunables = !q.iosched_tunables.empty();
            ++q.scheduler_generation;
        }

        uint64_t val = 0;
        if (parse_u64(read_first_line(q.queue_dir + "/read_ahead_kb"), val)) {
            q.active_read_ahead_kb = val;
            q.read_ahead_kb = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/nr_requests"), val)) {
            q.active_nr_requests = val;
            q.nr_requests = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/rq_affinity"), val)) {
            q.rq_affinity = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/iosched/read_expire"), val)) {
            q.read_expire_ms = val;
            q.has_iosched_tunables = true;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/iosched/writes_starved"), val)) {
            q.writes_starved = val;
        }
        if (parse_u64(read_first_line(q.queue_dir + "/iosched/fifo_batch"), val)) {
            q.fifo_batch = val;
        }
    };

    if (block_queues_.size() > 1U && worker_pool_ && worker_pool_->active()) {
        worker_pool_->parallel_for(block_queues_.size(), [&](size_t i) noexcept {
            refresh_queue(block_queues_[i]);
        });
    } else {
        for (auto& q : block_queues_) {
            refresh_queue(q);
        }
    }
    update_ucb_scores();
}

bool AutonomousTuner::verify_intervention_ownership(const ActiveIntervention& iv) const {
    if (iv.is_gpu_policy) {
        return gpu_frequency_controller_ != nullptr &&
               gpu_frequency_controller_->gpu_frame_policy_owned_intact();
    }
    if (iv.is_frequency_policy) {
        return frequency_actuator_ != nullptr && frequency_actuator_->mutation_active();
    }
    auto& store = BaselineStore::instance();
    for (const auto& step : iv.applied_steps) {
        if (::access(step.path.c_str(), R_OK) != 0 ||
            !store.owns_expected_state(step.path, step.target_value)) {
            return false;
        }
    }
    return true;
}

bool AutonomousTuner::verify_active_ownership() const {
    for (const auto& iv : active_interventions_) {
        if (!verify_intervention_ownership(iv)) {
            return false;
        }
    }
    auto& store = BaselineStore::instance();
    for (const auto& step : applied_steps_) {
        if (::access(step.path.c_str(), R_OK) != 0 ||
            !store.owns_expected_state(step.path, step.target_value)) {
            return false;
        }
    }
    return true;
}

bool AutonomousTuner::retry_contended_intervention_ownership(ActiveIntervention& iv) {
    if (iv.is_frequency_policy || iv.applied_steps.empty() ||
        iv.candidate_id.empty() || iv.candidate_id == "baseline" || recovery_required_) {
        return false;
    }
    uint32_t attempt = 0;
    constexpr uint32_t kMaximumContentionRetries = 3;
    if (!coordinator_.reserve_contention_retry(
            iv.mutation_domain, controller_name_,
            iv.candidate_id, kMaximumContentionRetries, &attempt)) {
        return false;
    }
    if (total_contention_retries_ < std::numeric_limits<uint32_t>::max()) {
        ++total_contention_retries_;
    }

    auto& store = BaselineStore::instance();
    for (const auto& step : iv.applied_steps) {
        if (!store.reassert_mutation_from_oem_baseline(step.path, step.target_value)) {
            return false;
        }
    }

    const uint32_t backoff = 1U << (attempt - 1U);
    iv.contention_backoff_ticks = backoff;
    contention_backoff_ticks_ = std::max(contention_backoff_ticks_, backoff);
    iv.obs_ticks = 0;
    iv.observing = true;
    iv.retained = false;
    iv.sum_after_io_psi = 0.0;
    iv.sum_after_mem_psi = 0.0;
    iv.sum_after_cpu_psi = 0.0;
    iv.sum_after_thermal = 0.0;
    iv.sum_after_temp_vel = 0.0;
    iv.valid_thermal_samples_in_window = 0;
    iv.valid_io_samples_in_window = 0;
    iv.valid_mem_samples_in_window = 0;
    iv.valid_cpu_samples_in_window = 0;
    iv.valid_pressure_samples_in_window = 0;
    iv.min_after_io_psi = 1e9;
    iv.min_after_mem_psi = 1e9;
    iv.min_after_cpu_psi = 1e9;
    iv.min_after_thermal = 1e9;
    iv.max_after_io_psi = -1e9;
    iv.max_after_mem_psi = -1e9;
    iv.max_after_cpu_psi = -1e9;
    iv.max_after_thermal = -1e9;
    if (const auto* stats = find_candidate(iv.candidate_id)) {
        iv.settling_ticks_remaining = stats->settling_ticks;
    }
    iv.verdict = "PENDING";
    iv.verdict_enum = TunerVerdict::Pending;
    iv.last_utility_delta = 0.0;
    iv.rationale = "OEM contention detected; exact captured OEM baseline reasserted " +
        std::to_string(attempt) + "/3; observation restarted after bounded tick backoff.";
    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    current_rationale_ = iv.rationale;
    sync_primary_state_from_active();
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::retry_contended_ownership() {
    if (active_interventions_.empty()) return false;
    return retry_contended_intervention_ownership(active_interventions_.front());
}

void AutonomousTuner::sync_primary_state_from_active() noexcept {
    if (active_interventions_.empty()) {
        transaction_active_ = false;
        observing_ = false;
        thermal_stabilization_active_ = false;
        current_evaluation_domain_ = EvaluationDomain::Unavailable;
        settling_ticks_remaining_ = 0;
        obs_ticks_ = 0;
        applied_steps_.clear();
        current_candidate_id_ = "baseline";
        current_candidate_label_ = "OEM Verified Baseline";
        current_intent_ = "HoldEquilibrium";
        return;
    }

    const ActiveIntervention* primary = &active_interventions_.front();
    bool any_observing = false;
    bool any_thermal_stab = false;
    for (const auto& iv : active_interventions_) {
        if (iv.observing && !any_observing) {
            primary = &iv;
        }
        if (iv.observing) any_observing = true;
        if (iv.thermal_stabilization) any_thermal_stab = true;
    }

    transaction_active_ = true;
    observing_ = any_observing;
    thermal_stabilization_active_ = any_thermal_stab;
    current_candidate_id_ = primary->candidate_id;
    current_candidate_label_ = primary->candidate_label;
    current_intent_ = primary->intent;
    current_rationale_ = primary->rationale;
    current_evaluation_domain_ = primary->evaluation_domain;
    settling_ticks_remaining_ = primary->settling_ticks_remaining;
    obs_ticks_ = primary->obs_ticks;
    obs_window_ = primary->obs_window;
    idle_streak_ = primary->idle_streak;
    before_io_psi_ = primary->before_io_psi;
    before_mem_psi_ = primary->before_mem_psi;
    before_cpu_psi_ = primary->before_cpu_psi;
    before_thermal_ = primary->before_thermal;
    before_temperature_celsius_ = primary->before_temperature_celsius;
    before_io_valid_ = primary->before_io_valid;
    before_mem_valid_ = primary->before_mem_valid;
    before_cpu_valid_ = primary->before_cpu_valid;
    before_thermal_valid_ = primary->before_thermal_valid;
    sum_after_io_psi_ = primary->sum_after_io_psi;
    sum_after_mem_psi_ = primary->sum_after_mem_psi;
    sum_after_cpu_psi_ = primary->sum_after_cpu_psi;
    sum_after_thermal_ = primary->sum_after_thermal;
    sum_after_temp_vel_ = primary->sum_after_temp_vel;
    valid_thermal_samples_in_window_ = primary->valid_thermal_samples_in_window;
    valid_io_samples_in_window_ = primary->valid_io_samples_in_window;
    valid_mem_samples_in_window_ = primary->valid_mem_samples_in_window;
    valid_cpu_samples_in_window_ = primary->valid_cpu_samples_in_window;
    valid_pressure_samples_in_window_ = primary->valid_pressure_samples_in_window;
    min_after_io_psi_ = primary->min_after_io_psi;
    max_after_io_psi_ = primary->max_after_io_psi;
    min_after_mem_psi_ = primary->min_after_mem_psi;
    max_after_mem_psi_ = primary->max_after_mem_psi;
    min_after_cpu_psi_ = primary->min_after_cpu_psi;
    max_after_cpu_psi_ = primary->max_after_cpu_psi;
    min_after_thermal_ = primary->min_after_thermal;
    max_after_thermal_ = primary->max_after_thermal;
    applied_steps_ = primary->applied_steps;
}

bool AutonomousTuner::rollback_partial_steps(
    MutationDomain domain,
    std::string_view candidate_id,
    std::vector<MutationStep>& steps,
    std::string_view reason) {
    auto& store = BaselineStore::instance();
    std::vector<MutationStep> failed_steps;
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
        if (!store.rollback(it->path)) {
            failed_steps.push_back(*it);
        }
    }
    if (!failed_steps.empty()) {
        std::reverse(failed_steps.begin(), failed_steps.end());
        steps = failed_steps;
        if (!recovery_required_) {
            if (auto* stats = find_candidate(candidate_id)) {
                ++stats->recovery_required_count;
            }
        }
        unresolved_recoveries_.push_back(FailedRecoveryRecord{
            .domain = domain,
            .candidate_id = std::string(candidate_id),
            .failed_steps = std::move(failed_steps),
            .is_frequency_policy = false,
        });
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        refresh_surface_readings();
        return false;
    }
    steps.clear();
    if (!candidate_id.empty() && candidate_id != "baseline") {
        coordinator_.release_lease(domain, controller_name_, candidate_id);
    }
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::rollback_intervention(ActiveIntervention& iv, std::string_view reason) {
    if (iv.is_gpu_policy) {
        if (gpu_frequency_controller_ == nullptr ||
            !gpu_frequency_controller_->restore_gpu_frame_policy()) {
            const bool already_recorded = std::any_of(unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
                [&](const FailedRecoveryRecord& rec) { return rec.is_gpu_policy && rec.candidate_id == iv.candidate_id; });
            if (!already_recorded) unresolved_recoveries_.push_back(FailedRecoveryRecord{
                .domain = MutationDomain::GpuDevfreq, .candidate_id = iv.candidate_id,
                .failed_steps = {}, .is_frequency_policy = false, .is_gpu_policy = true});
            recovery_required_ = true;
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            current_rationale_ = std::string(reason) + " [KGSL pair rollback unresolved; GpuDevfreq lease retained]";
            return false;
        }
        return true;
    }
    if (iv.is_frequency_policy) {
        if (frequency_actuator_ &&
            (frequency_mutation_active_ || frequency_actuator_->mutation_active() ||
             frequency_actuator_->recovery_required())) {
            if (!frequency_actuator_->rollback()) {
                frequency_mutation_active_ = true;
                const bool already_recorded = std::any_of(
                    unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
                    [&](const FailedRecoveryRecord& rec) {
                        return rec.is_frequency_policy;
                    });
                if (!already_recorded) {
                    unresolved_recoveries_.push_back(FailedRecoveryRecord{
                        .domain = MutationDomain::CpuPolicy,
                        .candidate_id = iv.candidate_id,
                        .failed_steps = {},
                        .is_frequency_policy = true,
                    });
                }
                recovery_required_ = true;
                last_verdict_ = "RECOVERY_REQUIRED";
                last_verdict_enum_ = TunerVerdict::RecoveryRequired;
                current_rationale_ = std::string(reason) +
                    " [frequency actuator rollback unresolved; CpuPolicy lease retained]";
                return false;
            }
            frequency_mutation_active_ = false;
        }
        return true;
    }

    auto& store = BaselineStore::instance();
    std::vector<MutationStep> failed_steps;
    // Unwind in reverse dependency order: Coupled-with dependents first, then Anchor.
    for (auto it = iv.applied_steps.rbegin(); it != iv.applied_steps.rend(); ++it) {
        if (!store.rollback(it->path)) {
            failed_steps.push_back(*it);
        }
    }

    if (!failed_steps.empty()) {
        std::reverse(failed_steps.begin(), failed_steps.end());
        iv.applied_steps = failed_steps;
        if (!recovery_required_) {
            if (auto* stats = find_candidate(iv.candidate_id)) {
                ++stats->recovery_required_count;
            }
        }
        auto existing = std::find_if(
            unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
            [&](const FailedRecoveryRecord& rec) {
                return !rec.is_frequency_policy &&
                       rec.domain == iv.mutation_domain &&
                       rec.candidate_id == iv.candidate_id;
            });
        if (existing != unresolved_recoveries_.end()) {
            existing->failed_steps = std::move(failed_steps);
        } else {
            unresolved_recoveries_.push_back(FailedRecoveryRecord{
                .domain = iv.mutation_domain,
                .candidate_id = iv.candidate_id,
                .failed_steps = std::move(failed_steps),
                .is_frequency_policy = false,
            });
        }
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        refresh_surface_readings();
        return false;
    }

    if (!iv.candidate_id.empty() && iv.candidate_id != "baseline") {
        coordinator_.release_lease(iv.mutation_domain, controller_name_, iv.candidate_id);
    }
    iv.applied_steps.clear();
    // Rebuild dynamic queue metadata after restoring the prior scheduler before
    // save_persisted_stats() recomputes the capability fingerprint.
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::retry_unresolved_recoveries() {
    auto& store = BaselineStore::instance();
    bool all_ok = true;
    for (auto it = unresolved_recoveries_.begin(); it != unresolved_recoveries_.end();) {
        if (it->is_gpu_policy) {
            if (gpu_frequency_controller_ == nullptr ||
                !gpu_frequency_controller_->restore_gpu_frame_policy()) {
                all_ok = false;
                ++it;
                continue;
            }
            it = unresolved_recoveries_.erase(it);
            continue;
        }
        if (it->is_frequency_policy) {
            if (frequency_actuator_ &&
                (frequency_actuator_->mutation_active() ||
                 frequency_actuator_->recovery_required())) {
                if (!frequency_actuator_->rollback()) {
                    all_ok = false;
                    ++it;
                    continue;
                }
            }
            frequency_mutation_active_ = false;
            it = unresolved_recoveries_.erase(it);
            continue;
        }

        std::vector<MutationStep> still_failed;
        for (auto step_it = it->failed_steps.rbegin(); step_it != it->failed_steps.rend(); ++step_it) {
            if (!store.rollback(step_it->path)) {
                still_failed.push_back(*step_it);
            }
        }
        if (!still_failed.empty()) {
            std::reverse(still_failed.begin(), still_failed.end());
            it->failed_steps = std::move(still_failed);
            all_ok = false;
            ++it;
            continue;
        }
        if (!it->candidate_id.empty() && it->candidate_id != "baseline") {
            coordinator_.release_lease(it->domain, controller_name_, it->candidate_id);
        }
        it = unresolved_recoveries_.erase(it);
    }

    if (frequency_actuator_ &&
        (frequency_mutation_active_ || frequency_actuator_->mutation_active() ||
         frequency_actuator_->recovery_required())) {
        if (!frequency_actuator_->rollback()) {
            frequency_mutation_active_ = true;
            all_ok = false;
        } else {
            frequency_mutation_active_ = false;
        }
    }
    return all_ok;
}

bool AutonomousTuner::rollback_active(std::string_view reason) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    contention_backoff_ticks_ = 0;

    bool all_restored = retry_unresolved_recoveries();

    // Also unwind any in-flight partial applied_steps_ (or pre-commit lease) left from an interrupted apply_transaction().
    if (active_interventions_.empty()) {
        if (!applied_steps_.empty()) {
            auto& store = BaselineStore::instance();
            std::vector<MutationStep> failed_steps;
            for (auto it = applied_steps_.rbegin(); it != applied_steps_.rend(); ++it) {
                if (!store.rollback(it->path)) {
                    failed_steps.push_back(*it);
                }
            }
            if (!failed_steps.empty()) {
                std::reverse(failed_steps.begin(), failed_steps.end());
                applied_steps_ = std::move(failed_steps);
                unresolved_recoveries_.push_back(FailedRecoveryRecord{
                    .domain = domain_for_candidate(current_candidate_id_),
                    .candidate_id = current_candidate_id_,
                    .failed_steps = applied_steps_,
                    .is_frequency_policy = false,
                });
                all_restored = false;
            } else {
                if (!current_candidate_id_.empty() && current_candidate_id_ != "baseline") {
                    coordinator_.release_lease(
                        domain_for_candidate(current_candidate_id_),
                        controller_name_,
                        current_candidate_id_);
                }
                applied_steps_.clear();
            }
        } else if (!current_candidate_id_.empty() && current_candidate_id_ != "baseline") {
            coordinator_.release_lease(
                domain_for_candidate(current_candidate_id_),
                controller_name_,
                current_candidate_id_);
        }
    }

    // Unwind all active multi-domain interventions in reverse order of acquisition.
    while (!active_interventions_.empty()) {
        auto iv = std::move(active_interventions_.back());
        active_interventions_.pop_back();
        if (!rollback_intervention(iv, reason)) {
            all_restored = false;
        }
    }

    observing_ = false;
    obs_ticks_ = 0;
    settling_ticks_remaining_ = 0;

    if (!all_restored || !unresolved_recoveries_.empty()) {
        transaction_active_ = false;
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) + " [ROLLBACK FAILED: RECOVERY_REQUIRED]";
        if (!unresolved_recoveries_.empty()) {
            current_candidate_id_ = unresolved_recoveries_.front().candidate_id;
            applied_steps_ = unresolved_recoveries_.front().failed_steps;
        }
        refresh_surface_readings();
        return false;
    }

    applied_steps_.clear();
    transaction_active_ = false;
    recovery_required_ = false;
    thermal_stabilization_active_ = false;
    current_evaluation_domain_ = EvaluationDomain::Unavailable;
    current_candidate_id_ = "baseline";
    current_candidate_label_ = "OEM Verified Baseline";
    current_intent_ = "HoldEquilibrium";
    current_rationale_ = std::string(reason);
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::emergency_rollback(std::string_view reason) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    auto& coordinator = coordinator_;
    auto& store = BaselineStore::instance();
    const bool had_active_transaction =
        !active_interventions_.empty() || transaction_active_;
    current_objective_ = OptimizationObjective::EmergencyMitigation;
    if (had_active_transaction) {
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
    }

    // Close admission before dispatching emergency preemption callbacks.
    coordinator.begin_emergency_recovery();
    coordinator.emergency_preempt_all(reason);
    if (frequency_actuator_ &&
        !frequency_actuator_->mutation_active() &&
        !frequency_actuator_->recovery_required()) {
        frequency_mutation_active_ = false;
    }
    const auto remaining = coordinator.active_leases();
    const bool live_owner_pending = std::any_of(
        remaining.begin(), remaining.end(),
        [](const DomainLease& lease) { return !lease.owner_detached; });
    if (live_owner_pending) {
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) +
            " [LIVE OWNER PREEMPTION ACKNOWLEDGEMENT PENDING]";
        save_persisted_stats();
        return false;
    }

    if (store.has_active_mutations()) (void)store.rollback_all();
    if (store.has_active_mutations() || !store.verify_and_persist_clear_state() ||
        (!remaining.empty() &&
         !coordinator.release_orphaned_leases_after_verified_recovery(store))) {
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) +
            " [ORPHANED OWNER RECOVERY INCOMPLETE; MUTATION ADMISSION REMAINS CLOSED]";
        save_persisted_stats();
        return false;
    }

    const bool restored = rollback_active(reason);
    if (!restored || store.has_active_mutations() ||
        !coordinator.complete_emergency_recovery()) {
        recovery_required_ = true;
        last_verdict_ = "RECOVERY_REQUIRED";
        last_verdict_enum_ = TunerVerdict::RecoveryRequired;
        current_rationale_ = std::string(reason) +
            " [GLOBAL RECOVERY INCOMPLETE; MUTATION ADMISSION REMAINS CLOSED]";
        save_persisted_stats();
        return false;
    }

    if (had_active_transaction) ++total_rolled_back_;
    current_intent_ = "ThermalHoldAndProtect";
    save_persisted_stats();
    return true;
}

bool AutonomousTuner::apply_transaction(
    std::string_view candidate_id,
    std::string_view candidate_label,
    std::string_view intent,
    std::string_view rationale,
    const std::vector<MutationStep>& steps,
    const TunerTelemetryInput& telemetry,
    EvaluationDomain eval_domain_override,
    const std::vector<std::string>& resource_keys,
    bool replace_existing_if_any) {
    if (steps.empty() || recovery_required_) return false;

    EvaluationDomain evaluation_domain = eval_domain_override;
    if (evaluation_domain == EvaluationDomain::Unavailable) {
        if (intent == "IoLatencyRelief") evaluation_domain = EvaluationDomain::Io;
        else if (intent == "MemoryStallRelief") evaluation_domain = EvaluationDomain::Memory;
        else if (intent == "InteractiveCpuPacing") evaluation_domain = EvaluationDomain::Cpu;
        else if (intent == "ThermalStabilization") evaluation_domain = EvaluationDomain::Thermal;
    }
    const bool target_baseline_valid =
        (evaluation_domain == EvaluationDomain::Io && telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Memory && telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Cpu && telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0) ||
        (evaluation_domain == EvaluationDomain::Thermal && telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0);
    if (!target_baseline_valid) return false;

    const MutationDomain dom = domain_for_candidate(candidate_id);
    if (replace_existing_if_any) {
        if (!rollback_active("Switching coordinated candidate")) {
            return false;
        }
    } else {
        // Ensure no same-domain intervention is active inside this tuner; rollback any same-domain stale entry.
        for (size_t idx = 0; idx < active_interventions_.size(); ++idx) {
            if (active_interventions_[idx].mutation_domain == dom) {
                if (!rollback_intervention(active_interventions_[idx], "Replacing same-domain candidate")) {
                    active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
                    sync_primary_state_from_active();
                    return false;
                }
                active_interventions_.erase(active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx));
                break;
            }
        }
    }

    auto& store = BaselineStore::instance();
    if (store.identity_unavailable()) return false;

    const ControllerPriority prio = priority_for_candidate(candidate_id);
    std::string conflict_reason;
    if (!coordinator_.acquire_lease(
            dom, controller_name_, candidate_id, prio, &conflict_reason)) {
        ++total_rejected_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->rejected_count;
            stats->last_rollback_reason = conflict_reason;
        }
        if (active_interventions_.empty()) {
            last_verdict_ = "REJECTED";
            last_verdict_enum_ = TunerVerdict::Rejected;
            current_rationale_ = "Candidate " + std::string(candidate_id) + " rejected: " + conflict_reason;
        }
        sync_primary_state_from_active();
        save_persisted_stats();
        return false;
    }

    // Publish the exact lease identity before preflight so preemption or failure unwinds cleanly.
    current_candidate_id_ = std::string(candidate_id);
    contention_backoff_ticks_ = 0;
    auto& coordinator = coordinator_;

    const auto reject_preflight = [&](std::string_view reason) {
        coordinator.release_lease(dom, controller_name_, candidate_id);
        ++total_rejected_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->rejected_count;
            stats->last_rollback_reason = std::string(reason);
        }
        if (active_interventions_.empty()) {
            last_verdict_ = "REJECTED";
            last_verdict_enum_ = TunerVerdict::Rejected;
            current_rationale_ = "Candidate " + std::string(candidate_id) +
                " rejected during preflight: " + std::string(reason);
        }
        sync_primary_state_from_active();
        save_persisted_stats();
    };

    const auto record_apply_abort = [&](std::string_view reason) {
        last_verdict_ = "ABORTED";
        last_verdict_enum_ = TunerVerdict::Aborted;
        ++total_rolled_back_;
        if (auto* stats = find_candidate(candidate_id)) {
            ++stats->aborted_count;
            stats->last_rollback_reason = std::string(reason);
        }
        current_rationale_ = std::string(reason);
    };

    // Phase 1: Full Preflight (All-or-Nothing for RequiredAnchor & RequiredDependent steps)
    std::vector<MutationStep> planned;
    planned.reserve(steps.size());
    bool has_anchor = false;
    std::string before_summary;

    for (const auto& step : steps) {
        const bool is_required = (step.relation == StepRelation::RequiredAnchor ||
                                  step.relation == StepRelation::RequiredDependent);
        if (!has_runtime_node(step.path)) {
            if (is_required) {
                reject_preflight("Node has no RUNTIME_TUNABLE contract: " + step.path);
                return false;
            }
            continue;
        }
        bool accessible = (::access(step.path.c_str(), R_OK | W_OK) == 0) &&
                          store.capture(step.path);
        if (accessible) {
            const auto audit = store.get_audit_trail();
            const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
                return node.path == step.path;
            });
            if (snapshot == audit.end() || snapshot->is_modified) accessible = false;
        }
        if (is_required && !accessible) {
            reject_preflight("Required resource unavailable, unsafe, or no longer writable: " + step.path);
            return false;
        }
        if (accessible) {
            if (step.relation == StepRelation::RequiredAnchor) {
                has_anchor = true;
            }
            if (!before_summary.empty()) before_summary += ",";
            before_summary += fs::path(step.path).filename().string() + "=" + read_first_line(step.path);
            planned.push_back(step);
        }
    }

    if (!has_anchor || planned.empty()) {
        reject_preflight("No valid required anchor or mutation steps were available");
        return false;
    }
    if (coordinator.is_preemption_requested(dom, controller_name_, candidate_id)) {
        return false;
    }

    // Phase 2: Ordered Execution (RequiredAnchor before RequiredDependent)
    std::vector<MutationStep> committed;
    committed.reserve(planned.size());
    std::string after_summary;

    for (const auto& step : planned) {
        if (coordinator.is_preemption_requested(dom, controller_name_, candidate_id)) {
            applied_steps_ = std::move(committed);
            observing_ = false;
            transaction_active_ = false;
            return false;
        }
        if (!store.apply_mutation(step.path, step.target_value)) {
            const bool is_required = (step.relation == StepRelation::RequiredAnchor ||
                                      step.relation == StepRelation::RequiredDependent);
            const auto audit = store.get_audit_trail();
            const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
                return node.path == step.path;
            });
            const bool unresolved_path_mutation =
                snapshot != audit.end() && snapshot->is_modified;

            if (!is_required) {
                if (unresolved_path_mutation && !store.rollback(step.path)) {
                    committed.push_back(step);
                    applied_steps_ = std::move(committed);
                    const std::string reason =
                        "Optional step failed and its path-scoped rollback is unresolved.";
                    record_apply_abort(reason);
                    if (active_interventions_.empty()) {
                        (void)rollback_active(reason);
                    } else {
                        (void)rollback_partial_steps(dom, candidate_id, applied_steps_, reason);
                        sync_primary_state_from_active();
                    }
                    save_persisted_stats();
                    return false;
                }
                continue;
            }

            if (unresolved_path_mutation) committed.push_back(step);
            applied_steps_ = std::move(committed);
            const std::string reason =
                "Transaction aborted on failed required step; rollback attempted.";
            record_apply_abort(reason);
            if (active_interventions_.empty()) {
                (void)rollback_active(reason);
            } else {
                (void)rollback_partial_steps(dom, candidate_id, applied_steps_, reason);
                sync_primary_state_from_active();
            }
            save_persisted_stats();
            return false;
        }
        if (!after_summary.empty()) after_summary += ",";
        after_summary += fs::path(step.path).filename().string() + "=" + step.target_value;
        committed.push_back(step);
    }

    if (coordinator.is_preemption_requested(dom, controller_name_, candidate_id)) {
        applied_steps_ = std::move(committed);
        observing_ = false;
        transaction_active_ = false;
        return false;
    }

    if (committed.empty()) {
        reject_preflight("No mutation step was committed");
        return false;
    }

    ActiveIntervention iv{};
    iv.candidate_id = std::string(candidate_id);
    iv.candidate_label = std::string(candidate_label);
    iv.mutation_domain = dom;
    iv.evaluation_domain = evaluation_domain;
    iv.intent = std::string(intent);
    iv.rationale = std::string(rationale);
    iv.is_frequency_policy = false;
    iv.thermal_stabilization = (intent == "ThermalStabilization");
    iv.observing = true;
    iv.retained = false;
    iv.obs_ticks = 0;
    iv.obs_window = obs_window_;
    iv.idle_streak = 0;
    iv.before_io_valid = telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0;
    iv.before_mem_valid = telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0;
    iv.before_cpu_valid = telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0;
    iv.before_thermal_valid = telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0;
    iv.before_io_psi = iv.before_io_valid ? telemetry.io_psi : 0.0;
    iv.before_mem_psi = iv.before_mem_valid ? telemetry.memory_psi : 0.0;
    iv.before_cpu_psi = iv.before_cpu_valid ? telemetry.cpu_psi : 0.0;
    iv.before_thermal = iv.before_thermal_valid ? telemetry.thermal_pressure : 0.0;
    iv.before_temperature_celsius = telemetry.current_temperature_celsius;
    iv.verdict = "PENDING";
    iv.verdict_enum = TunerVerdict::Pending;
    iv.last_utility_delta = 0.0;
    iv.applied_steps = std::move(committed);
    if (!resource_keys.empty()) {
        iv.resource_keys = resource_keys;
    } else {
        for (const auto& s : iv.applied_steps) {
            iv.resource_keys.push_back(s.path);
        }
    }

    noise_floor_ = evaluation_noise_floors_[static_cast<size_t>(evaluation_domain)];
    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    ++total_trials_;

    if (auto* stats = find_candidate(candidate_id)) {
        ++stats->trials;
        iv.settling_ticks_remaining = stats->settling_ticks;
        stats->last_workload = current_workload_;
        stats->last_thermal_bucket = compute_thermal_bucket(telemetry);
        stats->last_regime = current_regime_;
        stats->last_before_summary = std::move(before_summary);
        stats->last_after_summary = std::move(after_summary);
    } else {
        iv.settling_ticks_remaining = 1;
    }

    active_interventions_.push_back(std::move(iv));
    sync_primary_state_from_active();
    save_persisted_stats();
    refresh_surface_readings();
    return true;
}

bool AutonomousTuner::apply_gpu_proposal(
    std::string_view rationale,
    const TunerTelemetryInput& telemetry,
    const std::vector<std::string>& resource_keys) {
    constexpr std::string_view candidate_id = "gpu.kgsl.frame_deadline_relief";
    if (gpu_frequency_controller_ == nullptr || recovery_required_ ||
        !telemetry.gpu_busy_valid || !telemetry.frame_deadline_valid ||
        !telemetry.thermal_pressure_valid || telemetry.frame_deadline_window_end_ns == 0U ||
        !std::isfinite(telemetry.gpu_busy_pct) || telemetry.gpu_busy_pct < 70.0 ||
        telemetry.gpu_busy_pct > 100.0 || !std::isfinite(telemetry.frame_deadline_miss_pct) ||
        telemetry.frame_deadline_miss_pct < 0.5 || telemetry.frame_deadline_miss_pct > 100.0 ||
        telemetry.frame_deadline_count < 2U || telemetry.frame_deadline_misses == 0U ||
        telemetry.frame_deadline_misses > telemetry.frame_deadline_count ||
        !std::isfinite(telemetry.thermal_pressure) || telemetry.thermal_pressure < 0.0 ||
        telemetry.thermal_pressure > 130.0) return false;

    double thermal_headroom = telemetry.thermal_headroom;
    if (!std::isfinite(thermal_headroom) || thermal_headroom < 0.0) {
        thermal_headroom = std::clamp((88.0 - telemetry.thermal_pressure) / 46.0, 0.0, 1.0);
    }
    if (!std::isfinite(thermal_headroom) || thermal_headroom < 0.35 ||
        telemetry.thermal_guard_active || telemetry.thermal_emergency) return false;

    GpuFrequencySample before_frequency{};
    if (!gpu_frequency_controller_->read_gpu_frequency_sample(before_frequency)) return false;

    if (!gpu_frequency_controller_->apply_gpu_frame_policy(
            telemetry.gpu_busy_pct, telemetry.frame_deadline_miss_pct,
            thermal_headroom, false)) {
        if (!gpu_frequency_controller_->restore_gpu_frame_policy()) {
            recovery_required_ = true;
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            const bool recorded = std::any_of(unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
                [&](const FailedRecoveryRecord& rec) { return rec.is_gpu_policy && rec.candidate_id == candidate_id; });
            if (!recorded) unresolved_recoveries_.push_back(FailedRecoveryRecord{
                .domain = MutationDomain::GpuDevfreq, .candidate_id = std::string(candidate_id),
                .failed_steps = {}, .is_frequency_policy = false, .is_gpu_policy = true});
        }
        return false;
    }
    GpuFrequencySample after_frequency{};
    if (!gpu_frequency_controller_->gpu_frame_policy_owned_intact() ||
        !gpu_frequency_controller_->read_gpu_frequency_sample(after_frequency)) {
        if (!gpu_frequency_controller_->restore_gpu_frame_policy()) {
            recovery_required_ = true;
            last_verdict_ = "RECOVERY_REQUIRED";
            last_verdict_enum_ = TunerVerdict::RecoveryRequired;
            const bool recorded = std::any_of(unresolved_recoveries_.begin(), unresolved_recoveries_.end(),
                [&](const FailedRecoveryRecord& rec) { return rec.is_gpu_policy && rec.candidate_id == candidate_id; });
            if (!recorded) unresolved_recoveries_.push_back(FailedRecoveryRecord{
                .domain = MutationDomain::GpuDevfreq, .candidate_id = std::string(candidate_id),
                .failed_steps = {}, .is_frequency_policy = false, .is_gpu_policy = true});
        }
        return false;
    }

    ActiveIntervention iv{};
    iv.candidate_id = std::string(candidate_id);
    iv.candidate_label = "Qualcomm KGSL Frame-Deadline Recovery";
    iv.mutation_domain = MutationDomain::GpuDevfreq;
    iv.evaluation_domain = EvaluationDomain::Gpu;
    iv.intent = "GpuFrameDeadlineRelief";
    iv.rationale = std::string(rationale);
    iv.is_gpu_policy = true;
    iv.observing = true;
    iv.retained = false;
    iv.obs_ticks = 0;
    iv.obs_window = obs_window_;
    iv.before_gpu_valid = true;
    iv.before_gpu_miss_pct = telemetry.frame_deadline_miss_pct;
    iv.before_gpu_frequency_valid = true;
    iv.before_gpu_frequency_hz = before_frequency.current_frequency_hz;
    iv.sum_after_gpu_frequency_hz = static_cast<double>(after_frequency.current_frequency_hz);
    iv.min_after_gpu_frequency_hz = after_frequency.current_frequency_hz;
    iv.max_after_gpu_frequency_hz = after_frequency.current_frequency_hz;
    iv.valid_gpu_frequency_samples = 1U;
    iv.last_frame_deadline_window_end_ns = telemetry.frame_deadline_window_end_ns;
    iv.before_thermal_valid = true;
    iv.before_thermal = telemetry.thermal_pressure;
    iv.resource_keys = resource_keys;
    if (iv.resource_keys.empty()) iv.resource_keys.emplace_back("gpu:kgsl:pwrlevel-pair");
    iv.verdict = "PENDING";
    iv.verdict_enum = TunerVerdict::Pending;
    iv.settling_ticks_remaining = 1;

    ++total_trials_;
    if (auto* stats = find_candidate(candidate_id)) {
        ++stats->trials;
        stats->last_workload = current_workload_;
        stats->last_thermal_bucket = compute_thermal_bucket(telemetry);
        stats->last_regime = current_regime_;
        stats->last_before_summary = "frame_deadline_miss_pct=" +
            std::to_string(telemetry.frame_deadline_miss_pct) + "; gpuclk_hz=" +
            std::to_string(before_frequency.current_frequency_hz) + "; thermal_pressure=" +
            std::to_string(telemetry.thermal_pressure);
    }
    active_interventions_.push_back(std::move(iv));
    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    sync_primary_state_from_active();
    save_persisted_stats();
    return true;
}

bool AutonomousTuner::apply_frequency_proposal(
    double requested_capacity_scale,
    uint32_t policy_index,
    std::string_view rationale,
    const TunerTelemetryInput& telemetry,
    uint64_t policy_now_ns,
    const std::vector<std::string>& resource_keys) {
    if (frequency_actuator_ == nullptr || recovery_required_) return false;

    OutcomeSample baseline_sample{};
    baseline_sample.timestamp_ns = policy_now_ns;
    baseline_sample.cpu_psi_some_pct = telemetry.cpu_psi;
    baseline_sample.memory_psi_some_pct = telemetry.memory_psi;
    baseline_sample.io_psi_some_pct = telemetry.io_psi;
    baseline_sample.cpu_psi_valid = telemetry.cpu_psi_valid;
    baseline_sample.memory_psi_valid = telemetry.memory_psi_valid;
    baseline_sample.io_psi_valid = telemetry.io_psi_valid;
    const auto& features = telemetry.frequency_policy_evidence.telemetry;
    baseline_sample.ebpf_wakeup_rate_per_sec = features.wakeup_rate_per_sec;
    baseline_sample.ebpf_wakeup_valid = (features.valid_features & FeatureWakeupRate) != 0U;
    baseline_sample.workload_stable =
        telemetry.frequency_policy_evidence.classification.stable &&
        telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound;

    ++total_trials_;
    if (auto* stats = find_candidate("cpu.frequency.tiered")) {
        ++stats->trials;
        stats->last_workload = current_workload_;
        stats->last_thermal_bucket = compute_thermal_bucket(telemetry);
        stats->last_regime = current_regime_;
    }

    if (!frequency_actuator_->apply_policy(
            policy_index, requested_capacity_scale, baseline_sample, policy_now_ns)) {
        ++total_rejected_;
        if (auto* stats = find_candidate("cpu.frequency.tiered")) {
            ++stats->rejected_count;
        }
        last_verdict_ = "REJECTED";
        last_verdict_enum_ = TunerVerdict::Rejected;
        current_rationale_ =
            "Tiered CPU frequency request denied by live thermal, LMh, OPP, or ownership admission.";
        sync_primary_state_from_active();
        save_persisted_stats();
        return false;
    }

    frequency_mutation_active_ = true;
    frequency_policy_index_ = policy_index;
    ActiveIntervention iv{};
    iv.candidate_id = "cpu.frequency.tiered";
    iv.candidate_label = "Tiered CPU Frequency OPP";
    iv.mutation_domain = MutationDomain::CpuPolicy;
    iv.evaluation_domain = EvaluationDomain::Cpu;
    iv.intent = "TieredCpuFrequency";
    iv.rationale = std::string(rationale);
    iv.is_frequency_policy = true;
    iv.thermal_stabilization = false;
    iv.observing = true;
    iv.retained = false;
    iv.obs_ticks = 0;
    iv.obs_window = obs_window_;
    iv.before_io_psi = telemetry.io_psi_valid ? telemetry.io_psi : 0.0;
    iv.before_mem_psi = telemetry.memory_psi_valid ? telemetry.memory_psi : 0.0;
    iv.before_cpu_psi = telemetry.cpu_psi_valid ? telemetry.cpu_psi : 0.0;
    iv.before_thermal = telemetry.thermal_pressure_valid ? telemetry.thermal_pressure : 0.0;
    iv.before_temperature_celsius = telemetry.current_temperature_celsius;
    iv.before_io_valid = telemetry.io_psi_valid;
    iv.before_mem_valid = telemetry.memory_psi_valid;
    iv.before_cpu_valid = telemetry.cpu_psi_valid;
    iv.before_thermal_valid = telemetry.thermal_pressure_valid;
    iv.verdict = "PENDING";
    iv.verdict_enum = TunerVerdict::Pending;
    iv.resource_keys = resource_keys;

    last_verdict_ = "PENDING";
    last_verdict_enum_ = TunerVerdict::Pending;
    last_utility_delta_ = 0.0;
    active_interventions_.push_back(std::move(iv));
    sync_primary_state_from_active();
    save_persisted_stats();
    return true;
}

bool AutonomousTuner::evaluate_intervention_window(
    ActiveIntervention& iv,
    const TunerTelemetryInput& latest_telemetry) {
    if (!iv.observing || iv.obs_ticks == 0) return true;

    const auto mean = [](double sum, uint32_t count) noexcept {
        return count ? sum / static_cast<double>(count) : 0.0;
    };
    const double after_io = mean(iv.sum_after_io_psi, iv.valid_io_samples_in_window);
    const double after_mem = mean(iv.sum_after_mem_psi, iv.valid_mem_samples_in_window);
    const double after_cpu = mean(iv.sum_after_cpu_psi, iv.valid_cpu_samples_in_window);
    const double after_therm = mean(iv.sum_after_thermal, iv.valid_pressure_samples_in_window);
    const double after_gpu_miss = mean(iv.sum_after_gpu_miss_pct, iv.valid_gpu_samples_in_window);

    if (!verify_intervention_ownership(iv)) {
        if (retry_contended_intervention_ownership(iv)) {
            save_persisted_stats();
            return true;
        }
        last_verdict_ = "OWNERSHIP_LOST";
        last_verdict_enum_ = TunerVerdict::OwnershipLost;
        ++total_rolled_back_;
        record_ownership_loss(find_candidate(iv.candidate_id));
        (void)rollback_intervention(iv, "External node ownership lost during observation window.");
        save_persisted_stats();
        return false;
    }

    const uint32_t target_samples =
        iv.evaluation_domain == EvaluationDomain::Io ? iv.valid_io_samples_in_window :
        iv.evaluation_domain == EvaluationDomain::Memory ? iv.valid_mem_samples_in_window :
        iv.evaluation_domain == EvaluationDomain::Cpu ? iv.valid_cpu_samples_in_window :
        iv.evaluation_domain == EvaluationDomain::Gpu ? iv.valid_gpu_samples_in_window :
        iv.valid_pressure_samples_in_window;
    const bool gpu_outcome_complete = iv.evaluation_domain != EvaluationDomain::Gpu ||
        (iv.before_gpu_frequency_valid && iv.valid_gpu_frequency_samples >= 3U &&
         iv.before_thermal_valid && iv.valid_pressure_samples_in_window >= 2U);

    if (target_samples < 2 || !gpu_outcome_complete) {
        last_verdict_ = "INCONCLUSIVE";
        last_verdict_enum_ = TunerVerdict::Inconclusive;
        last_utility_delta_ = 0.0;
        iv.observing = false;
        iv.settling_ticks_remaining = 0;
        ++total_rolled_back_;
        ++total_inconclusive_;
        if (auto* stats = find_candidate(iv.candidate_id)) {
            ++stats->inconclusive_count;
            ++stats->consecutive_non_improved;
            stats->last_rollback_reason = "Insufficient valid target-domain samples";
        }
        (void)rollback_intervention(iv, "Target-domain evidence incomplete; reverted to OEM baseline.");
        update_ucb_scores();
        save_persisted_stats();
        return false;
    }

    const bool io_valid = iv.before_io_valid && iv.valid_io_samples_in_window >= 2;
    const bool mem_valid = iv.before_mem_valid && iv.valid_mem_samples_in_window >= 2;
    const bool cpu_valid = iv.before_cpu_valid && iv.valid_cpu_samples_in_window >= 2;
    const bool thermal_valid = iv.before_thermal_valid && iv.valid_pressure_samples_in_window >= 2;
    const bool gpu_valid = iv.before_gpu_valid && iv.valid_gpu_samples_in_window >= 2;
    const double gpu_gain = gpu_valid ? iv.before_gpu_miss_pct - after_gpu_miss : 0.0;
    const double io_gain = io_valid ? iv.before_io_psi - after_io : 0.0;
    const double mem_gain = mem_valid ? iv.before_mem_psi - after_mem : 0.0;
    const double cpu_gain = cpu_valid ? iv.before_cpu_psi - after_cpu : 0.0;
    const double therm_gain = thermal_valid ? iv.before_thermal - after_therm : 0.0;
    const double therm_penalty = thermal_valid ? std::max(0.0, after_therm - iv.before_thermal - 2.0) : 0.0;

    double utility_delta = 0.0;
    if (iv.intent == "ThermalStabilization") {
        const bool has_thermal_evidence =
            iv.valid_thermal_samples_in_window >= 2 && iv.valid_pressure_samples_in_window >= 2;
        if (!has_thermal_evidence ||
            (latest_telemetry.current_temperature_celsius < 0.0 && !latest_telemetry.temperature_velocity_valid)) {
            last_verdict_ = "INCONCLUSIVE";
            last_verdict_enum_ = TunerVerdict::Inconclusive;
            last_utility_delta_ = 0.0;
            iv.observing = false;
            iv.settling_ticks_remaining = 0;
            ++total_rolled_back_;
            ++total_inconclusive_;
            if (auto* stats = find_candidate(iv.candidate_id)) {
                ++stats->inconclusive_count;
                ++stats->consecutive_non_improved;
                stats->last_rollback_reason =
                    "Inconclusive: thermal sensor telemetry unavailable or insufficient samples";
                const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
                stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
            }
            const std::string msg =
                "Trial " + iv.candidate_id +
                " inconclusive due to missing thermal sensor evidence; reverted to OEM baseline.";
            (void)rollback_intervention(iv, msg);
            update_ucb_scores();
            save_persisted_stats();
            return false;
        }

        const double avg_temp_vel = (iv.valid_thermal_samples_in_window > 0)
            ? (iv.sum_after_temp_vel / static_cast<double>(iv.valid_thermal_samples_in_window))
            : latest_telemetry.temperature_velocity_celsius_per_sec;

        // Thermal outcomes are judged against both independent safety signals.
        // Neither a lower pressure reading nor a cooling trend may mask regression
        // in the other signal.
        const bool temperature_cooling = avg_temp_vel < -0.05;
        const bool temperature_not_warming = avg_temp_vel <= 0.05;
        const bool pressure_nonworsening = thermal_valid &&
            (after_therm <= iv.before_thermal + 0.50) &&
            ((iv.max_after_thermal - iv.min_after_thermal) <= 2.0);
        const bool thermal_objective_met =
            temperature_not_warming && pressure_nonworsening;

        // Preserve workload responsiveness: reject a thermal win that materially
        // worsens CPU/I/O contention or memory stalls during the same window.
        const bool workload_regressed =
            (io_valid && io_gain < -1.5) ||
            (cpu_valid && cpu_gain < -3.0) ||
            (mem_valid && mem_gain < -1.5);

        if (temperature_cooling && pressure_nonworsening) {
            const double cooling_gain = std::min(1.2, (-avg_temp_vel) * 1.5);
            utility_delta = 0.45 * therm_gain + cooling_gain +
                0.20 * io_gain + 0.15 * cpu_gain;
        } else if (thermal_objective_met) {
            utility_delta = std::max(0.0, therm_gain) * 0.40 +
                0.20 * io_gain + 0.15 * cpu_gain;
        } else {
            utility_delta = -0.50 - std::max(0.0, avg_temp_vel) * 1.5;
        }
        if (!thermal_objective_met || workload_regressed) {
            utility_delta = std::min(utility_delta, -0.25);
        }
    } else if (iv.intent == "IoLatencyRelief") {
        utility_delta = 1.40 * io_gain + (cpu_valid ? 0.35 * cpu_gain : 0.0) - 0.60 * therm_penalty;
    } else if (iv.intent == "MemoryStallRelief") {
        utility_delta = 1.60 * mem_gain + (io_valid ? 0.45 * io_gain : 0.0) - 0.60 * therm_penalty;
    } else if (iv.intent == "InteractiveCpuPacing") {
        utility_delta = 1.50 * cpu_gain + (io_valid ? 0.35 * io_gain : 0.0) - 0.65 * therm_penalty;
    } else if (iv.intent == "GpuFrameDeadlineRelief") {
        const double mean_gpu_frequency = mean(
            iv.sum_after_gpu_frequency_hz, iv.valid_gpu_frequency_samples);
        const double gpu_frequency_span = iv.valid_gpu_frequency_samples >= 2U && mean_gpu_frequency > 0.0
            ? static_cast<double>(iv.max_after_gpu_frequency_hz - iv.min_after_gpu_frequency_hz) / mean_gpu_frequency
            : 1.0;
        const double gpu_frequency_delta = iv.before_gpu_frequency_valid && iv.before_gpu_frequency_hz > 0U
            ? (mean_gpu_frequency - static_cast<double>(iv.before_gpu_frequency_hz)) /
                static_cast<double>(iv.before_gpu_frequency_hz)
            : 0.0;
        const double gpu_frequency_reward = 0.10 * std::clamp(gpu_frequency_delta, -0.25, 0.25);
        const double gpu_frequency_instability_penalty =
            std::clamp((gpu_frequency_span - 0.08) * 0.70, 0.0, 0.35);
        const bool gpu_frequency_regressed = gpu_frequency_span > 0.50;
        const bool gpu_thermal_regressed = thermal_valid &&
            (after_therm > iv.before_thermal + 4.0 || iv.max_after_thermal > iv.before_thermal + 8.0);
        utility_delta = gpu_valid && thermal_valid
            ? 1.50 * gpu_gain + gpu_frequency_reward - 0.65 * therm_penalty -
                gpu_frequency_instability_penalty
            : 0.0;
        if (gpu_frequency_regressed || gpu_thermal_regressed) {
            utility_delta = std::min(utility_delta, -0.75);
        }
    } else {
        utility_delta = 0.0;
    }

    iv.last_utility_delta = utility_delta;
    last_utility_delta_ = utility_delta;
    iv.observing = false;
    iv.settling_ticks_remaining = 0;

    auto* stats = find_candidate(iv.candidate_id);

    double target_spread = 0.0, target_baseline = 0.0, target_after = 0.0;
    std::string_view target_metric = "unavailable";
    size_t noise_index = 0;
    switch (iv.evaluation_domain) {
        case EvaluationDomain::Io:
            target_spread = iv.max_after_io_psi - iv.min_after_io_psi;
            target_baseline = iv.before_io_psi;
            target_after = after_io;
            target_metric = "io_psi";
            noise_index = 0;
            break;
        case EvaluationDomain::Memory:
            target_spread = iv.max_after_mem_psi - iv.min_after_mem_psi;
            target_baseline = iv.before_mem_psi;
            target_after = after_mem;
            target_metric = "memory_psi";
            noise_index = 1;
            break;
        case EvaluationDomain::Cpu:
            target_spread = iv.max_after_cpu_psi - iv.min_after_cpu_psi;
            target_baseline = iv.before_cpu_psi;
            target_after = after_cpu;
            target_metric = "cpu_psi";
            noise_index = 2;
            break;
        case EvaluationDomain::Thermal:
            target_spread = iv.max_after_thermal - iv.min_after_thermal;
            target_baseline = iv.before_thermal;
            target_after = after_therm;
            target_metric = "thermal_pressure";
            noise_index = 3;
            break;
        case EvaluationDomain::Gpu:
            target_spread = iv.max_after_gpu_miss_pct - iv.min_after_gpu_miss_pct;
            target_baseline = iv.before_gpu_miss_pct;
            target_after = after_gpu_miss;
            target_metric = "frame_deadline_miss_pct";
            noise_index = 0;
            break;
        default:
            break;
    }

    const double domain_weight = iv.evaluation_domain == EvaluationDomain::Gpu ? 0.08 :
        iv.evaluation_domain == EvaluationDomain::Io ? 0.07 :
        iv.evaluation_domain == EvaluationDomain::Memory ? 0.08 :
        iv.evaluation_domain == EvaluationDomain::Cpu ? 0.075 : 0.0225;
    const double measured_noise = std::clamp(target_spread * domain_weight, 0.02, 0.45);
    if (iv.evaluation_domain == EvaluationDomain::Gpu) {
        noise_floor_ = measured_noise;
    } else {
        evaluation_noise_floors_[noise_index] = std::clamp(
            0.85 * evaluation_noise_floors_[noise_index] + 0.15 * measured_noise, 0.02, 0.45);
        noise_floor_ = evaluation_noise_floors_[noise_index];
    }
    if (stats != nullptr && iv.evaluation_domain == EvaluationDomain::Gpu) {
        const double mean_gpu_frequency = mean(
            iv.sum_after_gpu_frequency_hz, iv.valid_gpu_frequency_samples);
        const double gpu_frequency_span_pct = mean_gpu_frequency > 0.0
            ? 100.0 * static_cast<double>(iv.max_after_gpu_frequency_hz - iv.min_after_gpu_frequency_hz) /
                mean_gpu_frequency
            : 0.0;
        stats->last_after_summary = "frame_deadline_miss_pct=" + std::to_string(after_gpu_miss).substr(0, 10) +
            "; gpuclk_mean_hz=" + std::to_string(mean_gpu_frequency).substr(0, 12) +
            "; gpuclk_span_pct=" + std::to_string(gpu_frequency_span_pct).substr(0, 8) +
            "; thermal_pressure=" + std::to_string(after_therm).substr(0, 8) + "; dU=" + std::to_string(utility_delta).substr(0, 8);
    }

    const double improved_threshold = std::max(0.25, 2.0 * noise_floor_);
    const double regressed_threshold = -(0.20 + noise_floor_);
    const bool high_variance_inconclusive =
        target_spread > std::max(6.0, std::abs(target_baseline) * 1.2) && std::abs(utility_delta) < 0.50;

    if (high_variance_inconclusive) {
        last_verdict_ = "INCONCLUSIVE";
        last_verdict_enum_ = TunerVerdict::Inconclusive;
        ++total_rolled_back_;
        ++total_inconclusive_;
        if (stats != nullptr) {
            ++stats->inconclusive_count;
            ++stats->consecutive_non_improved;
            stats->last_rollback_reason = "Inconclusive: sample variance exceeded noise floor";
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
        }
        const std::string msg =
            "Trial " + iv.candidate_id +
            " inconclusive due to high sample variance; reverted to OEM baseline.";
        (void)rollback_intervention(iv, msg);
        current_rationale_ = msg;
        update_ucb_scores();
        save_persisted_stats();
        return false;
    }

    ++total_reward_samples_;
    if (stats != nullptr) {
        const double previous = static_cast<double>(stats->reward_samples);
        ++stats->reward_samples;
        stats->mean_utility_delta = (stats->mean_utility_delta * previous + utility_delta) /
                                    static_cast<double>(stats->reward_samples);
    }

    bool keep_active = false;
    if (utility_delta >= improved_threshold) {
        iv.retained = true;
        iv.verdict = "IMPROVED";
        iv.verdict_enum = TunerVerdict::Improved;
        last_verdict_ = "IMPROVED";
        last_verdict_enum_ = TunerVerdict::Improved;
        ++total_improved_;
        if (stats != nullptr) {
            const auto regime_index = static_cast<std::size_t>(stats->last_regime);
            if (regime_index < stats->regime_reward_samples.size()) {
                const uint32_t samples = stats->regime_reward_samples[regime_index];
                const double reward = std::clamp(0.5 + utility_delta, 0.05, 2.50);
                ++stats->regime_reward_samples[regime_index];
                stats->regime_mean_utility[regime_index] = samples == 0U ? reward :
                    (stats->regime_mean_utility[regime_index] * static_cast<double>(samples) + reward) /
                    static_cast<double>(samples + 1U);
                if (stats->regime_cooldown_ticks[regime_index] > 0U) {
                    stats->regime_cooldown_ticks[regime_index] = 0U;
                }
            }
            ++stats->improved_count;
            stats->successes = stats->improved_count;
            stats->consecutive_non_improved = 0;
            stats->adaptive_scale = std::min(1.0, stats->adaptive_scale + 0.15);
            stats->mean_utility = std::clamp(
                stats->mean_utility + 0.30 * (std::min(2.5, utility_delta) - stats->mean_utility),
                0.05,
                2.50);
        }
        iv.rationale += " [Verified IMPROVED: ΔU=+" +
            std::to_string(utility_delta).substr(0, 4) +
            ", " + std::string(target_metric) + " " + std::to_string(target_baseline).substr(0, 5) +
            " -> " + std::to_string(target_after).substr(0, 5) + "% -> RETAINED]";
        current_rationale_ = iv.rationale;
        keep_active = true;
    } else if (utility_delta < regressed_threshold) {
        last_verdict_ = "REGRESSED";
        last_verdict_enum_ = TunerVerdict::Regressed;
        ++total_rolled_back_;
        if (stats != nullptr) {
            const auto regime_index = static_cast<std::size_t>(stats->last_regime);
            if (regime_index < stats->regime_reward_samples.size()) {
                const uint32_t samples = stats->regime_reward_samples[regime_index];
                const double reward = std::clamp(0.5 + utility_delta, 0.05, 2.50);
                ++stats->regime_reward_samples[regime_index];
                stats->regime_mean_utility[regime_index] = samples == 0U ? reward :
                    (stats->regime_mean_utility[regime_index] * static_cast<double>(samples) + reward) /
                    static_cast<double>(samples + 1U);
                ++stats->regime_regressions[regime_index];
                const uint32_t shift = std::min<uint32_t>(3U, stats->regime_regressions[regime_index] - 1U);
                stats->regime_cooldown_ticks[regime_index] = std::min<uint32_t>(64U, 8U * (1U << shift));
            }
            ++stats->regressed_count;
            stats->rollbacks = stats->regressed_count;
            ++stats->consecutive_non_improved;
            stats->adaptive_scale = std::max(0.25, stats->adaptive_scale * 0.75);
            stats->mean_utility = std::max(0.05, stats->mean_utility - 0.35);
            stats->last_rollback_reason = "Regressed: delta_U=" + std::to_string(utility_delta).substr(0, 5);
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(64U, 8U * (1U << shift));
        }
        const std::string msg =
            "Trial " + iv.candidate_id + " regressed (ΔU=" +
            std::to_string(utility_delta).substr(0, 5) +
            ", " + std::string(target_metric) + " " + std::to_string(target_baseline).substr(0, 5) +
            " -> " + std::to_string(target_after).substr(0, 5) +
            "%); rolled back to OEM baseline.";
        (void)rollback_intervention(iv, msg);
        current_rationale_ = msg;
        keep_active = false;
    } else {
        last_verdict_ = "NEUTRAL";
        last_verdict_enum_ = TunerVerdict::Neutral;
        ++total_rolled_back_;
        if (stats != nullptr) {
            const auto regime_index = static_cast<std::size_t>(stats->last_regime);
            if (regime_index < stats->regime_reward_samples.size()) {
                const uint32_t samples = stats->regime_reward_samples[regime_index];
                const double reward = std::clamp(0.5 + utility_delta, 0.05, 2.50);
                ++stats->regime_reward_samples[regime_index];
                stats->regime_mean_utility[regime_index] = samples == 0U ? reward :
                    (stats->regime_mean_utility[regime_index] * static_cast<double>(samples) + reward) /
                    static_cast<double>(samples + 1U);
                const uint32_t shift = std::min<uint32_t>(3U, stats->regime_regressions[regime_index]);
                stats->regime_cooldown_ticks[regime_index] = std::max<uint32_t>(
                    stats->regime_cooldown_ticks[regime_index], 4U * (1U << shift));
            }
            ++stats->neutral_count;
            ++stats->consecutive_non_improved;
            stats->adaptive_scale = std::max(0.50, stats->adaptive_scale * 0.90);
            stats->mean_utility = std::clamp(stats->mean_utility + 0.30 * (0.50 - stats->mean_utility), 0.05, 2.50);
            stats->last_rollback_reason = "Within noise floor; reverted per Invariant 8";
            const uint32_t shift = std::min<uint32_t>(3U, stats->consecutive_non_improved - 1U);
            stats->cooldown_ticks = std::min<uint32_t>(32U, 4U * (1U << shift));
        }
        const std::string msg =
            "Trial " + iv.candidate_id + " neutral (ΔU=" +
            std::to_string(utility_delta).substr(0, 4) +
            "); reverted to OEM baseline per Invariant 8 (no unverified retention).";
        (void)rollback_intervention(iv, msg);
        current_rationale_ = msg;
        keep_active = false;
    }

    update_ucb_scores();
    save_persisted_stats();
    return keep_active;
}

void AutonomousTuner::evaluate_observation_window(const TunerTelemetryInput& latest_telemetry) {
    if (active_interventions_.empty()) return;
    if (!evaluate_intervention_window(active_interventions_.front(), latest_telemetry)) {
        active_interventions_.erase(active_interventions_.begin());
    }
    sync_primary_state_from_active();
}

AutonomousDecisionSnapshot AutonomousTuner::step(const TunerTelemetryInput& raw_telemetry) {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    TunerTelemetryInput telemetry = raw_telemetry;
    latest_current_temperature_ = telemetry.current_temperature_celsius;
    latest_temperature_velocity_ = telemetry.temperature_velocity_celsius_per_sec;
    latest_temperature_velocity_valid_ = telemetry.temperature_velocity_valid;
    latest_pressure_velocity_ = telemetry.thermal_pressure_velocity;

    if (!operational_) {
        return snapshot();
    }

    if (!telemetry.temperature_velocity_valid && std::abs(telemetry.thermal_trend_celsius) > 1e-6) {
        telemetry.temperature_velocity_celsius_per_sec = telemetry.thermal_trend_celsius;
        telemetry.temperature_velocity_valid = true;
        telemetry.temperature_samples_count = std::max(telemetry.temperature_samples_count, 2U);
        latest_temperature_velocity_ = telemetry.temperature_velocity_celsius_per_sec;
        latest_temperature_velocity_valid_ = true;
    }

    if (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure)) {
        if (has_prev_thermal_ && std::abs(telemetry.thermal_pressure_velocity) < 1e-6) {
            telemetry.thermal_pressure_velocity = telemetry.thermal_pressure - prev_thermal_pressure_;
            latest_pressure_velocity_ = telemetry.thermal_pressure_velocity;
        }
        prev_thermal_pressure_ = telemetry.thermal_pressure;
        has_prev_thermal_ = true;
    } else {
        telemetry.thermal_pressure_velocity = 0.0;
        latest_pressure_velocity_ = 0.0;
    }

    const bool emergency_boundary =
        telemetry.thermal_emergency ||
        (telemetry.thermal_pressure_valid && telemetry.thermal_pressure >= 88.0);
    if (emergency_boundary) {
        current_objective_ = OptimizationObjective::EmergencyMitigation;
        coordinator_.emergency_preempt_all("ThermalEmergency");
    }

    if (recovery_required_) {
        if (!rollback_active("Retrying pending rollback recovery")) {
            return snapshot();
        }
    }

    bool cooldown_changed = false;
    for (auto& c : candidates_) {
        if (c.cooldown_ticks > 0) {
            --c.cooldown_ticks;
            cooldown_changed = true;
        }
        if (c.ownership_quarantine_ticks > 0) {
            --c.ownership_quarantine_ticks;
            cooldown_changed = true;
        }
        for (auto& regime_cooldown : c.regime_cooldown_ticks) {
            if (regime_cooldown > 0U) {
                --regime_cooldown;
                cooldown_changed = true;
            }
        }
    }
    if (cooldown_changed) {
        save_persisted_stats();
    }

    current_workload_ = classify_workload(telemetry);
    current_regime_ = classify_regime(telemetry, current_workload_);
    // Foreground render-thread migration is scoped to RenderBooster's reactor-owned
    // burst lease. timerfd expiry and thermal admission perform deterministic restore;
    // workload classification alone must not keep app threads migrated.

    if (emergency_boundary) {
        current_objective_ = OptimizationObjective::EmergencyMitigation;
        current_bottleneck_ = BottleneckKind::ThermalEmergency;
        if (!active_interventions_.empty() || frequency_mutation_active_ ||
            (frequency_actuator_ && frequency_actuator_->mutation_active())) {
            last_verdict_ = "ABORTED";
            last_verdict_enum_ = TunerVerdict::Aborted;
            for (const auto& iv : active_interventions_) {
                if (auto* stats = find_candidate(iv.candidate_id)) ++stats->aborted_count;
                ++total_rolled_back_;
            }
            if (active_interventions_.empty()) {
                ++total_rolled_back_;
            }
            (void)rollback_active("Thermal emergency gate active; restored OEM baseline.");
            save_persisted_stats();
        }
        current_intent_ = "ThermalHoldAndProtect";
        current_rationale_ = "Emergency thermal boundary reached; all domains locked to OEM baseline.";
        last_evaluated_candidates_.clear();
        last_decision_plan_rationale_ = current_rationale_;
        return snapshot();
    }

    // Evaluate optional Tiered CPU Frequency Policy input so it competes globally on utility.
    const uint64_t policy_now_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    const auto policy_index = select_frequency_policy_index();
    bool frequency_policy_evidence_ready = false;
    PolicyDecision frequency_decision{};
    if (frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr &&
        telemetry.frequency_policy_evidence_valid && policy_index.has_value() &&
        policy_now_ns != 0U && !recovery_required_ &&
        !coordinator_.is_emergency_recovery_active()) {
        PolicyEvidence policy_ev = telemetry.frequency_policy_evidence;
        policy_ev.evaluated_at_ns = policy_now_ns;
        policy_ev.mutation_runtime_ready = true;
        policy_ev.cpu_frequency_capability_valid =
            frequency_mutation_active_ ||
            frequency_actuator_->can_apply_policy(*policy_index, 1.02, policy_now_ns);
        frequency_decision = tiered_policy_router_->evaluate(platform_context_, policy_ev);
        frequency_policy_evidence_ready = true;
    }
    frequency_policy_decision_ = frequency_decision;
    const bool tiered_cpu_policy_owned =
        frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr &&
        telemetry.frequency_policy_evidence_valid &&
        telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound;
    const bool frequency_step_requested =
        frequency_policy_evidence_ready &&
        frequency_decision.action == PolicyAction::CpuFrequencyStep &&
        frequency_decision.requires_mutation_admission &&
        (frequency_decision.tier == OptimizationTier::Tier1InteractiveResponse ||
         frequency_decision.tier == OptimizationTier::Tier2SustainedOptimization);

    // 1. Global Environment Discovery & 2. Global Evidence Correlation
    const std::string vm_page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const std::string vm_cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    EnvironmentDiscoveryInput env_input{};
    env_input.sys_root = sys_root_;
    env_input.proc_root = proc_root_;
    env_input.zram_active = zram_active_;
    env_input.vm_page_cluster_path = vm_page_cluster_path;
    env_input.vm_vfs_cache_pressure_path = vm_cache_pressure_path;
    env_input.block_queues = &block_queues_;
    env_input.cpufreq_governor_dirs = &cpufreq_governor_dirs_;
    env_input.kernel_node_capabilities = &kernel_node_capabilities_;
    env_input.device_profile = device_profile_;
    env_input.frequency_actuator_configured =
        frequency_actuator_ != nullptr && tiered_policy_router_ != nullptr;
    env_input.gpu_actuator_ready = gpu_frequency_controller_ != nullptr &&
        gpu_frequency_controller_->kgsl_count() > 0U &&
        gpu_frequency_controller_->gpu_frequency_observation_ready();
    env_input.frequency_policy_index = policy_index;
    env_input.tiered_policy_decision = frequency_decision;
    env_input.tiered_policy_ready = frequency_policy_evidence_ready;
    env_input.tiered_cpu_policy_owned = tiered_cpu_policy_owned;
    env_input.uclamp_ready = (foreground_task_resolver_ != nullptr);

    const EnvironmentCapabilities env = EnvironmentDiscovery::discover(env_input);
    const bool in_thermal_recovery_before_step = (thermal_recovery_ticks_ > 0);
    const CorrelatedEvidence evidence =
        EvidenceCorrelator::correlate(telemetry, env, in_thermal_recovery_before_step);
    current_bottleneck_ = evidence.diagnosed_bottleneck;

    // Thermal Stabilization Arbitration (preempts non-stabilizing performance mutations)
    if (evidence.thermal_stabilization_required) {
        current_objective_ = OptimizationObjective::ThermalStabilization;
        thermal_recovery_ticks_ = 2;

        bool rolled_non_thermal = false;
        for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
            auto& iv = active_interventions_[idx - 1U];
            if (!iv.thermal_stabilization) {
                last_verdict_ = "ABORTED";
                last_verdict_enum_ = TunerVerdict::Aborted;
                if (auto* stats = find_candidate(iv.candidate_id)) ++stats->aborted_count;
                ++total_rolled_back_;
                (void)rollback_intervention(
                    iv, "Preempting performance mutation for active ThermalStabilization.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                rolled_non_thermal = true;
            }
        }
        if (rolled_non_thermal) {
            sync_primary_state_from_active();
            save_persisted_stats();
        }
        if (active_interventions_.empty()) {
            current_intent_ = "ThermalStabilization";
            current_rationale_ =
                "Thermal headroom constrained; switching objective to ThermalStabilization.";
        }
    } else if (telemetry.thermal_pressure_valid && !telemetry.thermal_guard_active) {
        bool rolled_thermal = false;
        for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
            auto& iv = active_interventions_[idx - 1U];
            if (iv.thermal_stabilization) {
                (void)rollback_intervention(
                    iv, "Thermal pressure subsided below hysteresis threshold; entering ThermalRecovery.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                rolled_thermal = true;
            }
        }
        if (rolled_thermal) {
            thermal_recovery_ticks_ = 2;
            sync_primary_state_from_active();
        }
    }

    const bool in_thermal_recovery = (thermal_recovery_ticks_ > 0);
    if (thermal_recovery_ticks_ > 0) {
        const bool temp_stable_or_falling =
            (!telemetry.temperature_velocity_valid) ||
            (telemetry.temperature_velocity_celsius_per_sec <= 0.05);
        if (telemetry.thermal_pressure_valid && telemetry.thermal_pressure <= 46.0 &&
            temp_stable_or_falling) {
            thermal_recovery_ticks_ = 0;
        } else {
            --thermal_recovery_ticks_;
        }
    }

    // Required-Evidence Completeness Gate (fail-closed per domain or globally when idle)
    if (!active_interventions_.empty()) {
        bool any_evidence_dropped = false;
        for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
            auto& iv = active_interventions_[idx - 1U];
            const bool domain_evidence_ok = iv.is_frequency_policy
                ? (telemetry.cpu_psi_valid && telemetry.memory_psi_valid &&
                   telemetry.io_psi_valid && telemetry.thermal_pressure_valid)
                : ((iv.evaluation_domain == EvaluationDomain::Io && telemetry.io_psi_valid) ||
                   (iv.evaluation_domain == EvaluationDomain::Memory && telemetry.memory_psi_valid) ||
                   (iv.evaluation_domain == EvaluationDomain::Cpu && telemetry.cpu_psi_valid) ||
                   (iv.evaluation_domain == EvaluationDomain::Thermal && telemetry.thermal_pressure_valid) ||
                   (iv.evaluation_domain == EvaluationDomain::Gpu && telemetry.gpu_busy_valid && telemetry.frame_deadline_valid));
            if (!domain_evidence_ok) {
                any_evidence_dropped = true;
                last_verdict_ = "INCONCLUSIVE";
                last_verdict_enum_ = TunerVerdict::Inconclusive;
                ++total_inconclusive_;
                ++total_rolled_back_;
                if (auto* stats = find_candidate(iv.candidate_id)) {
                    ++stats->inconclusive_count;
                    ++stats->consecutive_non_improved;
                    stats->last_rollback_reason = "Incomplete PSI evidence during observation/retention";
                }
                const bool restored = rollback_intervention(
                    iv, "Incomplete PSI evidence; releasing candidate and restoring OEM baseline.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                if (!restored) {
                    last_verdict_ = "RECOVERY_REQUIRED";
                    last_verdict_enum_ = TunerVerdict::RecoveryRequired;
                }
                save_persisted_stats();
            }
        }
        if (any_evidence_dropped && active_interventions_.empty()) {
            sync_primary_state_from_active();
            return snapshot();
        }
    } else if (!evidence.psi_complete && !evidence.thermal_stabilization_required) {
        current_objective_ = OptimizationObjective::HoldBaseline;
        current_intent_ = "HoldEquilibrium";
        current_rationale_ =
            "PSI evidence incomplete; workload mutation withheld while thermal policy remains active.";
        last_verdict_ = "INCONCLUSIVE";
        last_verdict_enum_ = TunerVerdict::Inconclusive;
        last_evaluated_candidates_.clear();
        last_decision_plan_rationale_ = current_rationale_;
        return snapshot();
    }

    current_objective_ = select_objective(
        telemetry, current_workload_, current_bottleneck_, in_thermal_recovery);

    if (contention_backoff_ticks_ > 0) {
        --contention_backoff_ticks_;
        return snapshot();
    }

    const bool io_bottleneck = telemetry.io_psi_valid && telemetry.io_psi >= 2.2;
    const bool mem_bottleneck = telemetry.memory_psi_valid && telemetry.memory_psi >= 2.0;

    // Stage 1 & 2: Advance all active interventions independently (OBSERVING / RETAINED / TieredFreq)
    bool ownership_lost_this_tick = false;
    bool handled_observation_tick = false;

    for (size_t idx = active_interventions_.size(); idx > 0; --idx) {
        auto& iv = active_interventions_[idx - 1U];

        if (iv.is_frequency_policy) {
            if (!frequency_step_requested || !policy_index.has_value() ||
                *policy_index != frequency_policy_index_) {
                (void)rollback_intervention(
                    iv, "Tiered CPU-bound evidence or independent admission no longer qualifies.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                if (!recovery_required_) {
                    last_verdict_ = "HOLD";
                    last_verdict_enum_ = TunerVerdict::Hold;
                    current_rationale_ =
                        "CPU frequency step released because Tier 1/Tier 2 evidence or admission expired.";
                }
                continue;
            }

            OutcomeSample sample{};
            sample.timestamp_ns = policy_now_ns;
            sample.cpu_psi_some_pct = telemetry.cpu_psi;
            sample.memory_psi_some_pct = telemetry.memory_psi;
            sample.io_psi_some_pct = telemetry.io_psi;
            sample.cpu_psi_valid = telemetry.cpu_psi_valid;
            sample.memory_psi_valid = telemetry.memory_psi_valid;
            sample.io_psi_valid = telemetry.io_psi_valid;
            const auto& features = telemetry.frequency_policy_evidence.telemetry;
            sample.ebpf_wakeup_rate_per_sec = features.wakeup_rate_per_sec;
            sample.ebpf_wakeup_valid = (features.valid_features & FeatureWakeupRate) != 0U;
            sample.workload_stable =
                telemetry.frequency_policy_evidence.classification.stable &&
                telemetry.frequency_policy_evidence.classification.workload == DetectedWorkloadClass::CpuBound;

            const OutcomeResult outcome = frequency_actuator_->observe(sample);
            frequency_mutation_active_ = frequency_actuator_->mutation_active();
            if (outcome.verdict == OutcomeVerdict::Pending) {
                iv.observing = true;
                iv.retained = false;
                iv.verdict = "PENDING";
                iv.verdict_enum = TunerVerdict::Pending;
                iv.rationale = std::string(frequency_decision.rationale);
                last_verdict_ = "PENDING";
                last_verdict_enum_ = TunerVerdict::Pending;
                handled_observation_tick = true;
            } else if (outcome.verdict == OutcomeVerdict::Improved && frequency_mutation_active_) {
                if (iv.verdict != "IMPROVED") {
                    ++total_reward_samples_;
                    ++total_improved_;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->reward_samples;
                        ++stats->improved_count;
                        stats->successes = stats->improved_count;
                    }
                }
                iv.observing = false;
                iv.retained = true;
                iv.verdict = "IMPROVED";
                iv.verdict_enum = TunerVerdict::Improved;
                iv.rationale = std::string(outcome.reason);
                last_verdict_ = "IMPROVED";
                last_verdict_enum_ = TunerVerdict::Improved;
            } else {
                if (outcome.verdict == OutcomeVerdict::Regressed) {
                    last_verdict_ = "REGRESSED";
                    last_verdict_enum_ = TunerVerdict::Regressed;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->reward_samples;
                        ++stats->regressed_count;
                        stats->rollbacks = stats->regressed_count;
                    }
                } else if (outcome.verdict == OutcomeVerdict::Neutral) {
                    last_verdict_ = "NEUTRAL";
                    last_verdict_enum_ = TunerVerdict::Neutral;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->reward_samples;
                        ++stats->neutral_count;
                    }
                } else {
                    last_verdict_ = "INCONCLUSIVE";
                    last_verdict_enum_ = TunerVerdict::Inconclusive;
                    ++total_inconclusive_;
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        ++stats->inconclusive_count;
                    }
                }
                if (!frequency_mutation_active_) ++total_rolled_back_;
                current_rationale_ = outcome.reason;
                if (frequency_actuator_->recovery_required()) {
                    (void)rollback_intervention(iv, outcome.reason);
                    recovery_required_ = true;
                    last_verdict_ = "RECOVERY_REQUIRED";
                    last_verdict_enum_ = TunerVerdict::RecoveryRequired;
                }
                if (!frequency_mutation_active_) {
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            }
            continue;
        }

        if (iv.observing) {
            handled_observation_tick = true;
            if (!verify_intervention_ownership(iv)) {
                if (retry_contended_intervention_ownership(iv)) {
                    save_persisted_stats();
                    continue;
                }
                last_verdict_ = "OWNERSHIP_LOST";
                last_verdict_enum_ = TunerVerdict::OwnershipLost;
                ++total_rolled_back_;
                record_ownership_loss(find_candidate(iv.candidate_id));
                (void)rollback_intervention(
                    iv, "External node ownership lost during observation window.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                save_persisted_stats();
                ownership_lost_this_tick = true;
                continue;
            }

            if (iv.settling_ticks_remaining > 0) {
                --iv.settling_ticks_remaining;
            }
            if (telemetry.io_psi_valid && std::isfinite(telemetry.io_psi) && telemetry.io_psi >= 0.0) {
                iv.sum_after_io_psi += telemetry.io_psi;
                iv.min_after_io_psi = std::min(iv.min_after_io_psi, telemetry.io_psi);
                iv.max_after_io_psi = std::max(iv.max_after_io_psi, telemetry.io_psi);
                ++iv.valid_io_samples_in_window;
            }
            if (telemetry.memory_psi_valid && std::isfinite(telemetry.memory_psi) && telemetry.memory_psi >= 0.0) {
                iv.sum_after_mem_psi += telemetry.memory_psi;
                iv.min_after_mem_psi = std::min(iv.min_after_mem_psi, telemetry.memory_psi);
                iv.max_after_mem_psi = std::max(iv.max_after_mem_psi, telemetry.memory_psi);
                ++iv.valid_mem_samples_in_window;
            }
            if (telemetry.cpu_psi_valid && std::isfinite(telemetry.cpu_psi) && telemetry.cpu_psi >= 0.0) {
                iv.sum_after_cpu_psi += telemetry.cpu_psi;
                iv.min_after_cpu_psi = std::min(iv.min_after_cpu_psi, telemetry.cpu_psi);
                iv.max_after_cpu_psi = std::max(iv.max_after_cpu_psi, telemetry.cpu_psi);
                ++iv.valid_cpu_samples_in_window;
            }
            if (telemetry.thermal_pressure_valid && std::isfinite(telemetry.thermal_pressure) && telemetry.thermal_pressure >= 0.0) {
                iv.sum_after_thermal += telemetry.thermal_pressure;
                iv.min_after_thermal = std::min(iv.min_after_thermal, telemetry.thermal_pressure);
                iv.max_after_thermal = std::max(iv.max_after_thermal, telemetry.thermal_pressure);
                ++iv.valid_pressure_samples_in_window;
            }
            if (telemetry.temperature_velocity_valid &&
                std::isfinite(telemetry.temperature_velocity_celsius_per_sec) &&
                telemetry.temperature_samples_count >= 2) {
                iv.sum_after_temp_vel += telemetry.temperature_velocity_celsius_per_sec;
                ++iv.valid_thermal_samples_in_window;
            }
            if (iv.evaluation_domain == EvaluationDomain::Gpu && telemetry.frame_deadline_valid &&
                telemetry.frame_deadline_window_end_ns != 0U &&
                telemetry.frame_deadline_window_end_ns > iv.last_frame_deadline_window_end_ns &&
                std::isfinite(telemetry.frame_deadline_miss_pct) &&
                telemetry.frame_deadline_miss_pct >= 0.0 && telemetry.frame_deadline_miss_pct <= 100.0) {
                iv.sum_after_gpu_miss_pct += telemetry.frame_deadline_miss_pct;
                iv.min_after_gpu_miss_pct = std::min(iv.min_after_gpu_miss_pct, telemetry.frame_deadline_miss_pct);
                iv.max_after_gpu_miss_pct = std::max(iv.max_after_gpu_miss_pct, telemetry.frame_deadline_miss_pct);
                ++iv.valid_gpu_samples_in_window;
                iv.last_frame_deadline_window_end_ns = telemetry.frame_deadline_window_end_ns;
            }
            if (iv.evaluation_domain == EvaluationDomain::Gpu && gpu_frequency_controller_ != nullptr) {
                GpuFrequencySample frequency_sample{};
                if (gpu_frequency_controller_->read_gpu_frequency_sample(frequency_sample)) {
                    if (iv.valid_gpu_frequency_samples == 0U) {
                        iv.min_after_gpu_frequency_hz = frequency_sample.current_frequency_hz;
                        iv.max_after_gpu_frequency_hz = frequency_sample.current_frequency_hz;
                    } else {
                        iv.min_after_gpu_frequency_hz = std::min(
                            iv.min_after_gpu_frequency_hz, frequency_sample.current_frequency_hz);
                        iv.max_after_gpu_frequency_hz = std::max(
                            iv.max_after_gpu_frequency_hz, frequency_sample.current_frequency_hz);
                    }
                    iv.sum_after_gpu_frequency_hz +=
                        static_cast<double>(frequency_sample.current_frequency_hz);
                    ++iv.valid_gpu_frequency_samples;
                }
            }
            ++iv.obs_ticks;
            if (iv.obs_ticks >= iv.obs_window) {
                if (!evaluate_intervention_window(iv, telemetry)) {
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            }
            continue;
        }

        if (iv.retained) {
            if (!verify_intervention_ownership(iv)) {
                if (retry_contended_intervention_ownership(iv)) {
                    save_persisted_stats();
                    continue;
                }
                last_verdict_ = "OWNERSHIP_LOST";
                last_verdict_enum_ = TunerVerdict::OwnershipLost;
                ++total_rolled_back_;
                record_ownership_loss(find_candidate(iv.candidate_id));
                (void)rollback_intervention(
                    iv, "External node ownership lost while transaction was retained.");
                active_interventions_.erase(
                    active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                save_persisted_stats();
                ownership_lost_this_tick = true;
                continue;
            }

            if (!io_bottleneck && !mem_bottleneck && telemetry.cpu_psi_valid &&
                telemetry.cpu_psi < 4.0 && !evidence.thermal_stabilization_required) {
                if (++iv.idle_streak >= 6) {
                    iv.idle_streak = 0;
                    last_verdict_ = "HOLD";
                    last_verdict_enum_ = TunerVerdict::Hold;
                    (void)rollback_intervention(
                        iv, "Workload returned to sustained idle; restored OEM baseline.");
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            } else {
                iv.idle_streak = 0;
                bool domain_regressed = false;
                switch (iv.evaluation_domain) {
                    case EvaluationDomain::Io:
                        domain_regressed = telemetry.io_psi_valid && iv.before_io_valid &&
                            telemetry.io_psi > iv.before_io_psi + 4.0;
                        break;
                    case EvaluationDomain::Memory:
                        domain_regressed = telemetry.memory_psi_valid && iv.before_mem_valid &&
                            telemetry.memory_psi > iv.before_mem_psi + 3.5;
                        break;
                    case EvaluationDomain::Cpu:
                        domain_regressed = telemetry.cpu_psi_valid && iv.before_cpu_valid &&
                            telemetry.cpu_psi > iv.before_cpu_psi + 6.0;
                        if (!domain_regressed && telemetry.thermal_pressure_valid && iv.before_thermal_valid) {
                            domain_regressed = telemetry.thermal_pressure > iv.before_thermal + 8.0;
                        }
                        break;
                    case EvaluationDomain::Thermal:
                        domain_regressed = telemetry.thermal_pressure_valid && iv.before_thermal_valid &&
                            telemetry.thermal_pressure > iv.before_thermal + 8.0;
                        if (!domain_regressed && telemetry.temperature_velocity_valid) {
                            domain_regressed = telemetry.temperature_velocity_celsius_per_sec > 0.05;
                        }
                        break;
                    case EvaluationDomain::Gpu:
                        domain_regressed = telemetry.frame_deadline_valid && iv.before_gpu_valid &&
                            telemetry.frame_deadline_miss_pct > iv.before_gpu_miss_pct + 1.0;
                        break;
                    default:
                        break;
                }
                if (domain_regressed) {
                    if (auto* stats = find_candidate(iv.candidate_id)) {
                        stats->cooldown_ticks = 6;
                        stats->last_rollback_reason = "Retained metric guard exceeded on workload shift";
                        save_persisted_stats();
                    }
                    (void)rollback_intervention(
                        iv, "Workload regime shifted; releasing retained candidate to re-plan.");
                    active_interventions_.erase(
                        active_interventions_.begin() + static_cast<std::ptrdiff_t>(idx - 1U));
                }
            }
        }
    }

    const bool any_still_observing = std::any_of(
        active_interventions_.begin(), active_interventions_.end(),
        [](const ActiveIntervention& iv) noexcept { return iv.observing; });

    // 3 & 4 & 5: Global Candidate Evaluation & Compatible Intervention Planning
    // Wait for any active observation window to complete before starting new trials
    // so causal before/after attribution across domains stays clean and unconfounded.
    if (!handled_observation_tick && !any_still_observing &&
        !ownership_lost_this_tick && !recovery_required_ && decision_engine_) {
        std::vector<ActiveDomainReservation> active_reservations;
        active_reservations.reserve(active_interventions_.size());
        for (const auto& iv : active_interventions_) {
            active_reservations.push_back(ActiveDomainReservation{
                .domain = iv.mutation_domain,
                .candidate_id = iv.candidate_id,
                .resource_keys = iv.resource_keys,
            });
        }

        const auto eligibility_fn = [&](const CandidateProposal& proposal, std::string& reason_out) -> bool {
            const CandidateStats* st = find_candidate(proposal.id);
            if (st == nullptr) {
                reason_out = "Unregistered candidate";
                return false;
            }
            if (st->ownership_quarantine_ticks > 0U) {
                reason_out = "Candidate quarantined after external ownership loss";
                return false;
            }
            if (!is_candidate_eligible_for_context(
                    proposal.id, evidence.workload, evidence.thermal_bucket)) {
                reason_out = "Cooldown, missing contract, hardware precondition, or coordinator lock";
                return false;
            }
            return true;
        };

        const InterventionPlan plan = decision_engine_->run_cycle(
            env,
            evidence,
            candidates_,
            total_reward_samples_,
            active_reservations,
            coordinator_,
            controller_name_,
            eligibility_fn,
            worker_pool_.get());

        last_evaluated_candidates_.clear();
        last_evaluated_candidates_.reserve(plan.ranked_all.size());
        for (const auto& item : plan.ranked_all) {
            const bool was_selected = std::any_of(
                plan.selected.begin(), plan.selected.end(),
                [&](const EvaluatedCandidate& sel) noexcept {
                    return sel.proposal.id == item.proposal.id;
                });
            last_evaluated_candidates_.push_back(EvaluatedCandidateSummary{
                .id = item.proposal.id,
                .domain = item.proposal.domain_label,
                .intent = item.proposal.intent,
                .utility_score = item.utility_score,
                .empirical_score = item.empirical_score,
                .confidence = item.confidence,
                .risk_penalty = item.risk_penalty,
                .thermal_penalty = item.thermal_penalty,
                .eligible = item.eligible,
                .selected = was_selected,
                .rejection_reason = item.rejection_reason,
            });
        }
        last_decision_plan_rationale_ = plan.planning_rationale;

        if (!plan.selected.empty()) {
            for (const auto& chosen : plan.selected) {
                if (chosen.proposal.action_kind == CandidateActionKind::GpuFrequencyPolicy) {
                    (void)apply_gpu_proposal(
                        chosen.proposal.rationale, telemetry, chosen.proposal.resource_keys);
                } else if (chosen.proposal.action_kind == CandidateActionKind::TieredFrequencyPolicy) {
                    (void)apply_frequency_proposal(
                        chosen.proposal.requested_capacity_scale,
                        chosen.proposal.frequency_policy_index,
                        chosen.proposal.rationale,
                        telemetry,
                        policy_now_ns,
                        chosen.proposal.resource_keys);
                } else {
                    std::vector<MutationStep> steps;
                    steps.reserve(chosen.proposal.steps.size());
                    for (const auto& s : chosen.proposal.steps) {
                        steps.push_back(MutationStep{
                            .path = s.path,
                            .target_value = s.target_value,
                            .relationship = s.relationship,
                            .relation = s.relation,
                        });
                    }
                    (void)apply_transaction(
                        chosen.proposal.id,
                        chosen.proposal.label,
                        chosen.proposal.intent,
                        chosen.proposal.rationale,
                        steps,
                        telemetry,
                        chosen.proposal.evaluation_domain,
                        chosen.proposal.resource_keys,
                        false);
                }
            }
        } else if (active_interventions_.empty() && in_thermal_recovery &&
                   !evidence.thermal_stabilization_required) {
            current_intent_ = "ThermalRecovery";
            current_rationale_ =
                "Hysteresis thermal recovery active; holding OEM baseline until thermal equilibrium stabilizes.";
        }
    }

    sync_primary_state_from_active();
    refresh_surface_readings();
    return snapshot();
}

AutonomousDecisionSnapshot AutonomousTuner::snapshot() const {
    std::lock_guard<std::recursive_mutex> lock(tuner_mutex_);
    AutonomousDecisionSnapshot out{};
    if (recovery_required_) {
        out.stage = "RECOVERY_REQUIRED";
    } else if (observing_ ||
               (frequency_mutation_active_ && last_verdict_ == "PENDING")) {
        out.stage = "OBSERVING";
    } else if (transaction_active_ ||
               (frequency_mutation_active_ && frequency_actuator_ &&
                frequency_actuator_->mutation_active())) {
        out.stage = "RETAINED";
    } else {
        out.stage = "PLANNED";
    }
    out.runtime_stage = operational_ ? "STAGE_ACTIVE_TUNING" : "STAGE_OBSERVING";
    out.active_intent = operational_ ? current_intent_ : "StartupBaselineObservation";
    out.intent = out.active_intent;
    out.workload_class = current_workload_;
    out.workload_regime = current_regime_;
    out.regime_name = std::string(regime_name(current_regime_));
    out.workload_name = std::string(workload_class_name(current_workload_));
    out.bottleneck = current_bottleneck_;
    out.bottleneck_name = std::string(bottleneck_kind_name(current_bottleneck_));
    out.objective = current_objective_;
    out.objective_name = std::string(objective_name(current_objective_));
    out.capability_fingerprint = compute_fingerprint();
    out.active_candidate_id = current_candidate_id_;
    out.active_candidate_label = current_candidate_label_;
    if (frequency_mutation_active_ && frequency_actuator_ &&
        frequency_actuator_->mutation_active() &&
        out.active_candidate_id == "baseline") {
        out.active_candidate_id = "cpu.frequency.tiered";
        out.active_candidate_label = "Tiered CPU Frequency OPP";
        out.active_intent = "TieredCpuFrequency";
        out.intent = "TieredCpuFrequency";
    }
    out.decision_rationale = current_rationale_;
    out.rationale = current_rationale_;
    out.last_verdict = last_verdict_;
    out.verdict = verdict_from_string(last_verdict_);
    out.last_utility_delta = last_utility_delta_;
    out.utility_delta = last_utility_delta_;
    out.noise_floor = noise_floor_;
    out.before_io_psi = before_io_psi_;
    out.after_io_psi = valid_io_samples_in_window_ > 0
        ? (sum_after_io_psi_ / static_cast<double>(valid_io_samples_in_window_))
        : before_io_psi_;
    out.before_mem_psi = before_mem_psi_;
    out.after_mem_psi = valid_mem_samples_in_window_ > 0
        ? (sum_after_mem_psi_ / static_cast<double>(valid_mem_samples_in_window_))
        : before_mem_psi_;
    out.before_cpu_psi = before_cpu_psi_;
    out.after_cpu_psi = valid_cpu_samples_in_window_ > 0
        ? (sum_after_cpu_psi_ / static_cast<double>(valid_cpu_samples_in_window_))
        : before_cpu_psi_;
    out.settling_ticks_remaining = settling_ticks_remaining_;
    out.observation_tick = obs_ticks_;
    out.observation_window = obs_window_;
    out.total_trials = total_trials_;
    out.total_improved = total_improved_;
    out.improved_count = total_improved_;
    out.total_rolled_back = total_rolled_back_;
    out.rolled_back_count = total_rolled_back_;
    out.total_inconclusive = total_inconclusive_;
    out.total_rejected = total_rejected_;
    out.ownership_contention_retries = total_contention_retries_;
    out.recovery_required = recovery_required_;
    out.thermal_stabilization_active = thermal_stabilization_active_;
    out.thermal_recovery_active = (thermal_recovery_ticks_ > 0);
    out.current_temperature_celsius = latest_current_temperature_;
    out.temperature_velocity_celsius_per_sec = latest_temperature_velocity_;
    out.temperature_velocity_valid = latest_temperature_velocity_valid_;
    out.thermal_pressure_velocity = latest_pressure_velocity_;
    out.block_queues = block_queues_;
    out.candidates = candidates_;
    out.discovered_resources = discovered_resources_;
    out.evaluated_candidates = last_evaluated_candidates_;
    out.decision_plan_rationale = last_decision_plan_rationale_;
    out.active_interventions.reserve(active_interventions_.size());
    for (const auto& iv : active_interventions_) {
        out.active_interventions.push_back(ActiveInterventionSummary{
            .candidate_id = iv.candidate_id,
            .candidate_label = iv.candidate_label,
            .domain = iv.mutation_domain,
            .evaluation_domain = iv.evaluation_domain,
            .intent = iv.intent,
            .stage = iv.observing ? "OBSERVING" : "RETAINED",
            .verdict = iv.verdict,
            .utility_delta = iv.last_utility_delta,
            .observation_tick = iv.obs_ticks,
            .observation_window = iv.obs_window,
            .settling_ticks_remaining = iv.settling_ticks_remaining,
        });
    }
    const auto page_cluster_path = resolve_vm_node_path("vm.page_cluster");
    const auto cache_pressure_path = resolve_vm_node_path("vm.vfs_cache_pressure");
    if (!page_cluster_path.empty()) out.vm_page_cluster = read_first_line(page_cluster_path);
    if (!cache_pressure_path.empty()) out.vm_vfs_cache_pressure = read_first_line(cache_pressure_path);
    return out;
}

} // namespace coreflow
`,t0=`/**
 * @file neural_predictor.cpp
 * @brief Hybrid ONNX Runtime C-API Bridge & Online RLS-MLP Thermal/Workload Predictor
 * @license MIT
 */
#include "coreflow/neural_predictor.hpp"
#include "coreflow/config.hpp"
#include "coreflow/unique_fd.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <dlfcn.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace {

// Minimal ABI subset of ONNX Runtime C API (OrtApiBase) to verify dynamic runtime presence
// without requiring heavyweight compile-time protobuf/onnx headers.
struct OrtApi;
struct OrtApiBase {
    const OrtApi* (*GetApi)(uint32_t version) noexcept;
    const char* (*GetVersionString)() noexcept;
};
using OrtGetApiBaseFn = const OrtApiBase* (*)() noexcept;

bool regular_file_exists(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    return ::fstat(fd.get(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}

} // namespace


NeuralPredictor::NeuralPredictor(std::string model_dir) : model_dir_(std::move(model_dir)) {
    initialize(model_dir_);
}

NeuralPredictor::~NeuralPredictor() {
    close_onnx_runtime();
}

void NeuralPredictor::init_default_network() noexcept {
    // Initialize 4x4 RLS covariance matrix P = 10 * I
    rls_cov_.fill(0.0);
    for (size_t i = 0; i < 4; ++i) {
        rls_cov_[i * 4 + i] = 10.0;
    }
    rls_weights_ = {0.0, 0.02, 0.14, -0.02};

    // Pre-calibrated 6 -> 8 -> 2 equilibrium network weights:
    // Inputs: [norm_pressure, norm_velocity, norm_cpu_psi, norm_mem_psi, norm_io_psi, norm_scale]
    w1_ = {
        // h0: thermal surge detector
         1.40,  1.10,  0.45,  0.10,  0.15,  0.30,
        // h1: sustained compute load detector
         0.50,  0.35,  1.25, -0.20, -0.15,  0.40,
        // h2: memory/IO stall contention detector
         0.20,  0.15,  0.10,  1.30,  1.20,  0.10,
        // h3: cool idle headroom detector
        -1.20, -0.80, -0.70, -0.50, -0.50, -0.20,
        // h4: thermal acceleration damper
         0.90,  1.50,  0.60,  0.25,  0.25,  0.50,
        // h5: interactive burst efficiency unit
        -0.40,  0.20,  0.95, -0.60, -0.40,  0.15,
        // h6: high-pressure saturation guard
         1.60,  0.80,  0.30,  0.70,  0.60,  0.45,
        // h7: recovery hysteresis unit
        -0.75, -0.95, -0.30, -0.20, -0.20,  0.10
    };
    b1_ = {-0.35, -0.20, -0.40, 0.25, -0.30, -0.10, -0.55, 0.15};

    // Output 0: non-linear pressure residual adjustment
    // Output 1: capacity scale modulation logit
    w2_ = {
        // Output 0 weights (8 hidden units)
         0.85,  0.40,  0.25, -0.60,  0.95,  0.15,  0.90, -0.50,
        // Output 1 weights (8 hidden units -> lower scale when thermal/stall units fire)
        -1.10, -0.25, -0.85,  0.70, -0.95,  0.35, -1.25,  0.65
    };
    b2_ = {0.05, 0.65};
}

void NeuralPredictor::reset() noexcept {
    init_default_network();
    prev_features_.fill(0.0);
    prev_hidden_.fill(0.0);
    prev_out0_ = 0.0;
    prev_pressure_ = -1.0;
    ema_abs_error_ = 1.5;
    samples_ = 0;
    last_ = NeuralPrediction{
        .valid = true,
        .onnx_active = onnx_loaded_,
        .predicted_pressure_10s = 45.0,
        .recommended_scale = 1.0,
        .confidence = 0.5,
        .prediction_mae = 0.0,
        .predicted_velocity = 0.0,
        .samples_observed = 0,
        .backend = onnx_loaded_ ? "onnx-runtime-hybrid"
                                : (custom_weights_loaded_ ? "custom-cfw-rls-mlp" : "embedded-rls-mlp"),
    };
}

void NeuralPredictor::close_onnx_runtime() noexcept {
    if (ort_lib_handle_ != nullptr) {
        (void)::dlclose(ort_lib_handle_);
        ort_lib_handle_ = nullptr;
    }
    onnx_loaded_ = false;
}

bool NeuralPredictor::try_load_custom_weights(const std::string& path) noexcept {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!fd.valid()) return false;
    struct stat st{};
    if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 16384) {
        return false;
    }

    std::string content;
    content.reserve(static_cast<size_t>(st.st_size));
    char buf[4096];
    while (content.size() < 16384U) {
        const size_t remaining = 16384U - content.size();
        const ssize_t n = ::read(fd.get(), buf, std::min(sizeof(buf), remaining));
        if (n > 0) {
            content.append(buf, static_cast<size_t>(n));
            continue;
        }
        if (n == 0) break;
        if (errno == EINTR) continue;
        return false;
    }
    if (content.empty()) return false;

    std::istringstream stream(content);
    std::string header;
    if (!(stream >> header) || header != "CFW1") return false;

    std::array<double, 48> cand_w1{};
    std::array<double, 8> cand_b1{};
    std::array<double, 16> cand_w2{};
    std::array<double, 2> cand_b2{};

    for (double& v : cand_w1) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_b1) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_w2) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }
    for (double& v : cand_b2) {
        if (!(stream >> v) || !std::isfinite(v) || std::abs(v) > 25.0) return false;
    }

    w1_ = cand_w1;
    b1_ = cand_b1;
    w2_ = cand_w2;
    b2_ = cand_b2;
    return true;
}

bool NeuralPredictor::try_load_onnx_runtime(const std::string& onnx_path) noexcept {
    if (!regular_file_exists(onnx_path)) return false;

    constexpr const char* kLibCandidates[] = {
        "/data/adb/coreflow/lib/libonnxruntime.so",
        "/system/lib64/libonnxruntime.so",
        "/vendor/lib64/libonnxruntime.so",
        "libonnxruntime.so",
    };

    for (const char* candidate : kLibCandidates) {
        void* handle = ::dlopen(candidate, RTLD_NOW | RTLD_LOCAL);
        if (handle == nullptr) continue;

        auto* sym = ::dlsym(handle, "OrtGetApiBase");
        if (sym == nullptr) {
            (void)::dlclose(handle);
            continue;
        }

        OrtGetApiBaseFn get_api_base = nullptr;
        *reinterpret_cast<void**>(&get_api_base) = sym;
        const OrtApiBase* base = get_api_base ? get_api_base() : nullptr;
        if (base != nullptr && base->GetVersionString != nullptr) {
            ort_lib_handle_ = handle;
            onnx_loaded_ = true;
            return true;
        }
        (void)::dlclose(handle);
    }
    return false;
}

void NeuralPredictor::initialize(std::string_view model_dir) noexcept {
    model_dir_ = std::string(model_dir);
    close_onnx_runtime();
    custom_weights_loaded_ = false;
    (void)try_load_onnx_runtime(model_dir_ + "/equilibrium.onnx");
    reset();
    custom_weights_loaded_ = try_load_custom_weights(model_dir_ + "/equilibrium.cfw");
    last_.backend = onnx_loaded_ ? "onnx-runtime-hybrid"
                                 : (custom_weights_loaded_ ? "custom-cfw-rls-mlp" : "embedded-rls-mlp");
}

NeuralPrediction NeuralPredictor::infer_and_learn(const NeuralInput& input) noexcept {
    const bool finite = std::isfinite(input.thermal_pressure) &&
                        (!input.cpu_psi_valid || std::isfinite(input.cpu_psi)) &&
                        (!input.memory_psi_valid || std::isfinite(input.memory_psi)) &&
                        (!input.io_psi_valid || std::isfinite(input.io_psi)) &&
                        std::isfinite(input.current_scale) &&
                        std::isfinite(input.elapsed_seconds);
    if (!finite || input.elapsed_seconds <= 0.0 || input.elapsed_seconds > 120.0 ||
        input.thermal_pressure < 0.0 || input.thermal_pressure > 130.0) {
        NeuralPrediction invalid{};
        invalid.valid = false;
        return invalid;
    }

    const double dt = std::clamp(input.elapsed_seconds, 0.05, 30.0);
    const double norm_p = std::clamp((input.thermal_pressure - 60.0) / 40.0, -1.5, 2.0);
    const double norm_cpu = input.cpu_psi_valid ? std::clamp(input.cpu_psi / 50.0, 0.0, 2.0) : 0.0;
    const double norm_mem = input.memory_psi_valid ? std::clamp(input.memory_psi / 25.0, 0.0, 2.0) : 0.0;
    const double norm_io = input.io_psi_valid ? std::clamp(input.io_psi / 25.0, 0.0, 2.0) : 0.0;
    const bool pressure_inputs_valid =
        input.cpu_psi_valid && input.memory_psi_valid && input.io_psi_valid;
    const double norm_scale = std::clamp((input.current_scale - 0.4) / 0.6, 0.0, 1.0);

    double velocity = 0.0;

    // Step 1: Online Recursive Least Squares (RLS) + NLMS Output-Layer learning step from previous tick
    if (pressure_inputs_valid && prev_pressure_ >= 0.0) {
        const double actual_rate = (input.thermal_pressure - prev_pressure_) / dt;
        velocity = std::clamp(actual_rate, -10.0, 10.0);

        double predicted_rate = 0.0;
        for (size_t i = 0; i < 4; ++i) {
            predicted_rate += rls_weights_[i] * prev_features_[i];
        }
        const double innovation = std::clamp(actual_rate - predicted_rate, -8.0, 8.0);
        ema_abs_error_ = 0.88 * ema_abs_error_ + 0.12 * std::abs(innovation);

        // Compute P * x
        std::array<double, 4> px{};
        for (size_t i = 0; i < 4; ++i) {
            double sum = 0.0;
            for (size_t j = 0; j < 4; ++j) {
                sum += rls_cov_[i * 4 + j] * prev_features_[j];
            }
            px[i] = sum;
        }

        constexpr double kForgettingFactor = 0.96;
        double denom = kForgettingFactor;
        for (size_t i = 0; i < 4; ++i) {
            denom += prev_features_[i] * px[i];
        }
        if (denom > 1e-6) {
            std::array<double, 4> gain{};
            for (size_t i = 0; i < 4; ++i) {
                gain[i] = px[i] / denom;
                rls_weights_[i] = std::clamp(rls_weights_[i] + gain[i] * innovation, -5.0, 5.0);
            }
            // Update covariance P = (P - k * x^T * P) / lambda
            std::array<double, 16> next_cov{};
            for (size_t i = 0; i < 4; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    const double updated = (rls_cov_[i * 4 + j] - gain[i] * px[j]) / kForgettingFactor;
                    next_cov[i * 4 + j] = std::clamp(updated, -25.0, 25.0);
                }
            }
            rls_cov_ = next_cov;
        }

        // Online NLMS adaptation on Micro-MLP output head (w2_[0..7], b2_[0])
        // Refines non-linear thermal surge residual without destabilizing pre-calibrated hidden features
        const double tanh_out0 = std::tanh(prev_out0_);
        const double grad_scale = (1.0 - tanh_out0 * tanh_out0);
        const double residual_err = std::clamp(innovation / 8.0 - tanh_out0, -1.0, 1.0);
        constexpr double kMlpStep = 0.015;
        for (size_t h = 0; h < 8; ++h) {
            w2_[h] = std::clamp(w2_[h] + kMlpStep * residual_err * grad_scale * prev_hidden_[h], -3.0, 3.0);
        }
        b2_[0] = std::clamp(b2_[0] + kMlpStep * residual_err * grad_scale, -1.5, 1.5);
    }

    const std::array<double, 4> current_rls_features{
        1.0,
        norm_p,
        std::clamp(norm_cpu + 0.5 * norm_io, 0.0, 2.5),
        norm_scale,
    };
    if (pressure_inputs_valid) {
        prev_features_ = current_rls_features;
        prev_pressure_ = input.thermal_pressure;
    } else {
        // Do not train across an interval with missing workload-pressure features.
        prev_pressure_ = -1.0;
    }
    ++samples_;

    // Step 2: RLS 10-second linear horizon projection
    double rls_rate_forecast = 0.0;
    for (size_t i = 0; i < 4; ++i) {
        rls_rate_forecast += rls_weights_[i] * current_rls_features[i];
    }
    rls_rate_forecast = std::clamp(rls_rate_forecast, -4.0, 4.0);

    // Step 3: 6 -> 8 -> 2 Micro-MLP non-linear forward pass
    const std::array<double, 6> mlp_in{
        norm_p,
        std::clamp(velocity / 2.0, -2.0, 2.0),
        norm_cpu,
        norm_mem,
        norm_io,
        norm_scale,
    };

    std::array<double, 8> hidden{};
    for (size_t h = 0; h < 8; ++h) {
        double acc = b1_[h];
        for (size_t k = 0; k < 6; ++k) {
            acc += w1_[h * 6 + k] * mlp_in[k];
        }
        hidden[h] = std::tanh(acc);
    }
    prev_hidden_ = hidden;

    std::array<double, 2> out{b2_[0], b2_[1]};
    for (size_t o = 0; o < 2; ++o) {
        for (size_t h = 0; h < 8; ++h) {
            out[o] += w2_[o * 8 + h] * hidden[h];
        }
    }
    prev_out0_ = out[0];

    const double nonlinear_pressure_delta = std::tanh(out[0]) * 8.0;
    const double predicted_10s = std::clamp(
        input.thermal_pressure + rls_rate_forecast * 6.0 + nonlinear_pressure_delta,
        0.0,
        130.0);

    // Sigmoid activation mapped to [0.40, 1.00]
    const double sigmoid_scale = 1.0 / (1.0 + std::exp(-std::clamp(out[1], -8.0, 8.0)));
    const double recommended_scale = std::clamp(0.40 + 0.60 * sigmoid_scale, 0.40, 1.00);

    const double warmup_factor = std::min(1.0, static_cast<double>(samples_) / 5.0);
    const double error_confidence = 1.0 / (1.0 + 0.35 * ema_abs_error_);
    const double confidence = pressure_inputs_valid
        ? std::clamp(0.35 + 0.65 * warmup_factor * error_confidence, 0.10, 0.99)
        : std::min(0.35, std::clamp(0.35 + 0.65 * warmup_factor * error_confidence, 0.10, 0.99));

    last_ = NeuralPrediction{
        .valid = true,
        .onnx_active = onnx_loaded_,
        .predicted_pressure_10s = predicted_10s,
        .recommended_scale = recommended_scale,
        .confidence = confidence,
        .prediction_mae = ema_abs_error_,
        .predicted_velocity = rls_rate_forecast,
        .samples_observed = samples_,
        .backend = onnx_loaded_ ? "onnx-runtime-hybrid"
                                : (custom_weights_loaded_ ? "custom-cfw-rls-mlp" : "embedded-rls-mlp"),
    };
    return last_;
}

NeuralPrediction NeuralPredictor::last_prediction() const noexcept {
    return last_;
}

} // namespace coreflow
`,n0=`#include "coreflow/frequency_controller.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/autonomous_tuner.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;
namespace {
bool number_allow_zero(const std::string& path, uint64_t& value) {
    std::ifstream file(path);
    std::string text;
    if (!(file >> text)) return false;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
bool number(const std::string& path, uint64_t& value) {
    return number_allow_zero(path, value) && value > 0U;
}
bool verified_regular_writable(const std::string& path) noexcept {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode) &&
        (st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0 && ::access(path.c_str(), W_OK) == 0;
}
bool snapshot_is_oem(const std::vector<NodeSnapshot>& audit, const std::string& path,
                     uint64_t value) {
    const auto found = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == path;
    });
    return found != audit.end() && !found->is_modified &&
        found->factory_value == std::to_string(value);
}
}
FrequencyController::FrequencyController(CentralMutationCoordinator& coordinator, std::string root, DeviceProfileHandle profile)
    : coordinator_(coordinator), root_(std::move(root)), profile_(std::move(profile)),
      controller_name_(CentralMutationCoordinator::make_controller_identity("FrequencyController")) {
    (void)coordinator_.register_preemption_handler(
        controller_name_,
        [this](MutationDomain domain, std::string_view transaction_id, std::string_view) noexcept {
            return handle_preemption(domain, transaction_id);
        });
}
FrequencyController::~FrequencyController() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cpu_transaction_id_.empty()) (void)rollback_domain(false);
        if (!gpu_transaction_id_.empty()) (void)rollback_domain(true);
    }
    coordinator_.unregister_preemption_handler(controller_name_);
}
bool FrequencyController::acquire_domain_lease(bool gpu, bool thermal_guard) {
    auto& transaction_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    const auto domain = gpu ? MutationDomain::GpuDevfreq : MutationDomain::CpuPolicy;
    if (!transaction_id.empty()) {
        return coordinator_.acquire_lease(
            domain, controller_name_, transaction_id,
            thermal_guard ? ControllerPriority::ThermalStabilization
                          : ControllerPriority::WorkloadOptimization);
    }
    transaction_id = "frequency-" + std::string(gpu ? "gpu-" : "cpu-") +
                     std::to_string(++lease_generation_);
    if (!coordinator_.acquire_lease(
            domain, controller_name_, transaction_id,
            thermal_guard ? ControllerPriority::ThermalStabilization
                          : ControllerPriority::WorkloadOptimization)) {
        transaction_id.clear();
        return false;
    }
    return true;
}
void FrequencyController::release_domain_lease(bool gpu) noexcept {
    auto& transaction_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    if (transaction_id.empty()) return;
    const auto domain = gpu ? MutationDomain::GpuDevfreq : MutationDomain::CpuPolicy;
    coordinator_.release_lease(
        domain, controller_name_, transaction_id);
    transaction_id.clear();
}
bool FrequencyController::rollback_domain(bool gpu) {
    auto& store = BaselineStore::instance();
    for (auto& item : domains_) {
        if (item.gpu != gpu || !item.mutation_active) continue;
        if (item.kgsl_power_levels) {
            uint64_t actual_max = 0U, actual_min = 0U;
            const std::string expected_max = std::to_string(item.last);
            const std::string expected_min = std::to_string(item.last_minimum);
            if (!number_allow_zero(item.maximum, actual_max) ||
                !number_allow_zero(item.minimum, actual_min) ||
                actual_max != item.last || actual_min != item.last_minimum ||
                actual_max > actual_min || actual_min > item.max_valid_level ||
                !store.owns_expected_state(item.maximum, expected_max) ||
                !store.owns_expected_state(item.minimum, expected_min)) {
                // A changed inode/value is an ownership handoff. Never overwrite
                // a foreign KGSL limit while trying to release our paired lease.
                return false;
            }

            const auto rollback_pair_node = [&](const std::string& path,
                                                uint64_t expected_value,
                                                uint64_t other_value) {
                if (!store.rollback(path)) return false;
                uint64_t read_max = 0U, read_min = 0U;
                if (!number_allow_zero(item.maximum, read_max) ||
                    !number_allow_zero(item.minimum, read_min) ||
                    read_max > read_min || read_min > item.max_valid_level) return false;
                if (path == item.maximum) {
                    return read_max == expected_value && read_min == other_value;
                }
                return read_min == expected_value && read_max == other_value;
            };

            // Both endpoints are restored in an order that preserves the KGSL
            // invariant after each individual sysfs write. A two-node sysfs pair
            // cannot be physically atomic, so the coordinator lease + WAL +
            // intermediate readback form the transactional boundary.
            if (item.baseline <= actual_min) {
                if (!rollback_pair_node(item.maximum, item.baseline, actual_min) ||
                    !rollback_pair_node(item.minimum, item.baseline_minimum, item.baseline)) {
                    return false;
                }
            } else if (actual_max <= item.baseline_minimum) {
                if (!rollback_pair_node(item.minimum, item.baseline_minimum, actual_max) ||
                    !rollback_pair_node(item.maximum, item.baseline, item.baseline_minimum)) {
                    return false;
                }
            } else {
                return false;
            }

            uint64_t restored_max = 0U, restored_min = 0U;
            if (!number_allow_zero(item.maximum, restored_max) ||
                !number_allow_zero(item.minimum, restored_min) ||
                restored_max != item.baseline ||
                restored_min != item.baseline_minimum ||
                restored_max > restored_min || restored_min > item.max_valid_level) return false;
            const auto audit = store.get_audit_trail();
            if (!snapshot_is_oem(audit, item.maximum, restored_max) ||
                !snapshot_is_oem(audit, item.minimum, restored_min) ||
                !store.owns_expected_state(item.maximum, std::to_string(restored_max)) ||
                !store.owns_expected_state(item.minimum, std::to_string(restored_min))) return false;
            item.last = restored_max;
            item.last_minimum = restored_min;
            item.mutation_active = false;
            continue;
        }
        if (!store.rollback(item.maximum)) return false;
        uint64_t actual = 0;
        if (!number(item.maximum, actual)) return false;
        const auto audit = store.get_audit_trail();
        const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == item.maximum;
        });
        if (snapshot == audit.end() || snapshot->is_modified ||
            snapshot->factory_value != std::to_string(actual)) return false;
        if (actual != item.baseline) {
            item.baseline = actual;
            item.retired = true;
        }
        item.last = actual;
        item.mutation_active = false;
    }
    release_domain_lease(gpu);
    return true;
}
bool FrequencyController::handle_preemption(
    MutationDomain domain, std::string_view transaction_id) noexcept {
    const bool gpu = domain == MutationDomain::GpuDevfreq;
    if (domain != MutationDomain::CpuPolicy && !gpu) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& active_id = gpu ? gpu_transaction_id_ : cpu_transaction_id_;
    if (active_id.empty() || active_id != transaction_id) return false;
    return rollback_domain(gpu);
}
void FrequencyController::add(const std::string& dir, bool gpu) {
    const auto max = dir + (gpu ? "/max_freq" : "/scaling_max_freq");
    const auto min = dir + (gpu ? "/min_freq" : "/scaling_min_freq");
    uint64_t ceiling = 0, floor = 0;
    if (!number(max, ceiling) || !number(min, floor) || floor > ceiling || ::access(max.c_str(), W_OK) != 0) return;
    std::ifstream table(dir + (gpu ? "/available_frequencies" : "/scaling_available_frequencies"));
    std::vector<uint64_t> values;
    uint64_t frequency;
    while (table >> frequency) {
        if (frequency > 0 && frequency <= ceiling) values.push_back(frequency);
        if (values.size() > 512) return;
    }
    if (!table.eof() || values.empty()) return;
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    auto& store = BaselineStore::instance();
    if (!store.capture(max)) return;
    const auto audit = store.get_audit_trail();
    const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == max;
    });
    // A pre-existing journaled mutation has no ownership relationship to this
    // newly discovered controller instance. Do not adopt it as an OEM baseline.
    if (snapshot == audit.end() || snapshot->is_modified) return;
    Domain domain{};
    domain.maximum = max;
    domain.minimum = min;
    domain.baseline = ceiling;
    domain.last = ceiling;
    domain.frequencies = std::move(values);
    domain.gpu = gpu;
    domains_.push_back(std::move(domain));
}

void FrequencyController::add_kgsl(const std::string& dir) {
    const std::string maximum = dir + "/max_pwrlevel";
    const std::string minimum = dir + "/min_pwrlevel";
    const std::string count_path = dir + "/num_pwrlevels";
    const std::string table_path = dir + "/gpu_available_frequencies";
    uint64_t current_max = 0U, current_min = 0U, reported_levels = 0U;
    if (!number_allow_zero(maximum, current_max) || !number_allow_zero(minimum, current_min) ||
        !number_allow_zero(count_path, reported_levels) ||
        !verified_regular_writable(maximum) || !verified_regular_writable(minimum)) return;
    std::ifstream table(table_path);
    std::vector<uint64_t> frequencies;
    uint64_t frequency = 0U;
    while (table >> frequency) {
        if (frequency == 0U || frequencies.size() >= 512U) return;
        frequencies.push_back(frequency);
    }
    if (!table.eof() || frequencies.size() < 2U || reported_levels != frequencies.size()) return;
    const uint64_t table_last = static_cast<uint64_t>(frequencies.size() - 1U);
    if (reported_levels != frequencies.size()) return;
    if (current_max > current_min || current_min > table_last) return;
    const bool increasing = std::adjacent_find(frequencies.begin(), frequencies.end(),
        [](uint64_t a, uint64_t b) { return b <= a; }) == frequencies.end();
    const bool decreasing = std::adjacent_find(frequencies.begin(), frequencies.end(),
        [](uint64_t a, uint64_t b) { return b >= a; }) == frequencies.end();
    if (!increasing && !decreasing) return;

    auto& store = BaselineStore::instance();
    if (!store.capture(maximum) || !store.capture(minimum)) return;
    const auto audit = store.get_audit_trail();
    if (!snapshot_is_oem(audit, maximum, current_max) ||
        !snapshot_is_oem(audit, minimum, current_min)) return;
    Domain domain{};
    domain.maximum = maximum;
    domain.minimum = minimum;
    domain.current_frequency = dir + "/gpuclk";
    domain.baseline = current_max;
    domain.last = current_max;
    domain.baseline_minimum = current_min;
    domain.last_minimum = current_min;
    domain.max_valid_level = table_last;
    domain.frequencies = std::move(frequencies);
    domain.gpu = true;
    domain.kgsl_power_levels = true;
    domains_.push_back(std::move(domain));
}

bool FrequencyController::apply_gpu_frame_policy(
    double gpu_busy_pct, double frame_miss_pct, double thermal_headroom,
    bool thermal_guard) {
    if (!std::isfinite(gpu_busy_pct) || gpu_busy_pct < 0.0 || gpu_busy_pct > 100.0 ||
        !std::isfinite(frame_miss_pct) || frame_miss_pct < 0.0 || frame_miss_pct > 100.0 ||
        !std::isfinite(thermal_headroom) || thermal_headroom < 0.0 || thermal_headroom > 1.0) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const bool has_kgsl = std::any_of(domains_.begin(), domains_.end(), [](const Domain& item) {
        return item.gpu && item.kgsl_power_levels && !item.retired;
    });
    if (!has_kgsl) return false;

    if (thermal_guard || thermal_headroom < 0.35 || gpu_busy_pct < 70.0 || frame_miss_pct < 0.5) {
        return rollback_domain(true);
    }
    if (!acquire_domain_lease(true, false)) return false;

    const uint64_t step = gpu_busy_pct >= 90.0 && frame_miss_pct >= 5.0 &&
        thermal_headroom >= 0.55 ? 2U : 1U;
    auto& store = BaselineStore::instance();
    for (auto& item : domains_) {
        if (!item.gpu || !item.kgsl_power_levels || item.retired) continue;
        uint64_t actual_max = 0U, actual_min = 0U;
        if (!number_allow_zero(item.maximum, actual_max) ||
            !number_allow_zero(item.minimum, actual_min) || actual_max > actual_min ||
            actual_min > item.max_valid_level || actual_max != item.last ||
            actual_min != item.last_minimum ||
            !store.owns_expected_state(item.maximum, std::to_string(actual_max)) ||
            !store.owns_expected_state(item.minimum, std::to_string(actual_min))) {
            item.retired = true;
            if (item.mutation_active && !rollback_domain(true)) return false;
            continue;
        }

        const auto audit = store.get_audit_trail();
        const auto max_node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == item.maximum;
        });
        const auto min_node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == item.minimum;
        });
        const bool baseline_pair_valid =
            max_node != audit.end() && min_node != audit.end() &&
            max_node->factory_value == std::to_string(item.baseline) &&
            min_node->factory_value == std::to_string(item.baseline_minimum) &&
            (item.mutation_active ||
             (!max_node->is_modified && !min_node->is_modified));
        if (!baseline_pair_valid) {
            item.retired = true;
            if (item.mutation_active && !rollback_domain(true)) return false;
            continue;
        }

        const uint64_t reducible = item.baseline_minimum > item.baseline
            ? item.baseline_minimum - item.baseline : 0U;
        const uint64_t target_max = item.baseline;
        const uint64_t target_min = std::max(item.baseline,
            item.baseline_minimum - std::min(step, reducible));
        if (target_max > target_min || target_min > item.max_valid_level) {
            item.retired = true;
            if (item.mutation_active && !rollback_domain(true)) return false;
            continue;
        }
        if (target_max == actual_max && target_min == actual_min) continue;

        // The max/min pair shares one GpuDevfreq lease and two durable baseline
        // records. Mark the pair active before the first endpoint write so any
        // partial failure routes through the same deterministic pair rollback.
        item.mutation_active = true;
        uint64_t working_max = actual_max;
        uint64_t working_min = actual_min;
        const auto write_max = [&]() {
            if (working_max == target_max) return true;
            if (target_max > working_min) return false;
            if (!store.apply_mutation(item.maximum, std::to_string(target_max))) return false;
            uint64_t read_max = 0U, read_min = 0U;
            if (!number_allow_zero(item.maximum, read_max) ||
                !number_allow_zero(item.minimum, read_min) ||
                read_max != target_max || read_min != working_min ||
                read_max > read_min || read_min > item.max_valid_level) return false;
            working_max = read_max;
            item.last = read_max;
            item.last_minimum = read_min;
            return true;
        };
        const auto write_min = [&]() {
            if (working_min == target_min) return true;
            if (working_max > target_min) return false;
            if (!store.apply_mutation(item.minimum, std::to_string(target_min))) return false;
            uint64_t read_max = 0U, read_min = 0U;
            if (!number_allow_zero(item.maximum, read_max) ||
                !number_allow_zero(item.minimum, read_min) ||
                read_max != working_max || read_min != target_min ||
                read_max > read_min || read_min > item.max_valid_level) return false;
            working_max = read_max;
            working_min = read_min;
            item.last = read_max;
            item.last_minimum = read_min;
            return true;
        };

        // If tightening max first would cross the current min, lower min first.
        // Otherwise max can be committed first. In either order the ABI invariant
        // remains true between writes.
        bool applied = true;
        if (target_max != working_max && target_max > working_min) {
            applied = write_min();
        }
        if (applied && target_max != working_max) applied = write_max();
        if (applied && target_min != working_min) applied = write_min();

        uint64_t read_max = 0U, read_min = 0U;
        const bool readback = applied &&
            number_allow_zero(item.maximum, read_max) &&
            number_allow_zero(item.minimum, read_min) &&
            read_max == target_max && read_min == target_min &&
            read_max <= read_min && read_min <= item.max_valid_level;
        const auto after_audit = store.get_audit_trail();
        const auto owns_endpoint = [&](const std::string& path, uint64_t value, uint64_t baseline) {
            const auto node = std::find_if(after_audit.begin(), after_audit.end(), [&](const NodeSnapshot& entry) {
                return entry.path == path;
            });
            const std::string expected = std::to_string(value);
            return node != after_audit.end() &&
                node->factory_value == std::to_string(baseline) &&
                node->active_value == expected &&
                node->is_modified == (value != baseline) &&
                store.owns_expected_state(path, expected);
        };
        const bool owned = readback &&
            owns_endpoint(item.maximum, read_max, item.baseline) &&
            owns_endpoint(item.minimum, read_min, item.baseline_minimum);
        if (!owned) {
            // Keep the latest verified pair for the ownership-aware rollback path.
            uint64_t observed_max = 0U, observed_min = 0U;
            if (number_allow_zero(item.maximum, observed_max) &&
                number_allow_zero(item.minimum, observed_min) &&
                observed_max <= observed_min && observed_min <= item.max_valid_level) {
                item.last = observed_max;
                item.last_minimum = observed_min;
            }
            if (!rollback_domain(true)) return false;
            return false;
        }
        item.last = read_max;
        item.last_minimum = read_min;
    }
    const bool active = std::any_of(domains_.begin(), domains_.end(), [](const Domain& item) {
        return item.gpu && item.kgsl_power_levels && item.mutation_active;
    });
    if (!active) release_domain_lease(true);
    return active;
}
bool FrequencyController::restore_gpu_frame_policy() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_domain(true);
}

bool FrequencyController::gpu_frame_policy_owned_intact() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    bool observed_active_pair = false;
    const auto& store = BaselineStore::instance();
    for (const auto& item : domains_) {
        if (!item.gpu || !item.kgsl_power_levels || !item.mutation_active) continue;
        observed_active_pair = true;
        uint64_t actual_max = 0U, actual_min = 0U;
        if (!number_allow_zero(item.maximum, actual_max) ||
            !number_allow_zero(item.minimum, actual_min) ||
            actual_max != item.last || actual_min != item.last_minimum ||
            actual_max > actual_min || actual_min > item.max_valid_level) return false;
        const auto audit = store.get_audit_trail();
        const auto owned_endpoint = [&](const std::string& path, uint64_t value, uint64_t baseline) {
            const auto node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& entry) {
                return entry.path == path;
            });
            const std::string expected = std::to_string(value);
            return node != audit.end() &&
                node->factory_value == std::to_string(baseline) &&
                node->active_value == expected &&
                node->is_modified == (value != baseline) &&
                store.owns_expected_state(path, expected);
        };
        if (!owned_endpoint(item.maximum, actual_max, item.baseline) ||
            !owned_endpoint(item.minimum, actual_min, item.baseline_minimum)) return false;
    }
    return observed_active_pair;
}

bool FrequencyController::read_gpu_frequency_sample(GpuFrequencySample& sample) const noexcept {
    sample = {};
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& store = BaselineStore::instance();
    size_t observed_domains = 0U;
    for (const auto& item : domains_) {
        if (!item.gpu || !item.kgsl_power_levels || item.retired) continue;
        ++observed_domains;
        if (observed_domains != 1U) return false; // No ambiguous multi-GPU aggregate.

        uint64_t actual_max = 0U, actual_min = 0U, actual_frequency = 0U;
        if (!number_allow_zero(item.maximum, actual_max) ||
            !number_allow_zero(item.minimum, actual_min) ||
            !number(item.current_frequency, actual_frequency) ||
            actual_max != item.last || actual_min != item.last_minimum ||
            actual_max > actual_min || actual_min > item.max_valid_level ||
            !store.owns_expected_state(item.maximum, std::to_string(actual_max)) ||
            !store.owns_expected_state(item.minimum, std::to_string(actual_min))) return false;
        const auto opp = std::find(item.frequencies.begin(), item.frequencies.end(), actual_frequency);
        if (opp == item.frequencies.end()) return false;
        const uint64_t opp_index = static_cast<uint64_t>(std::distance(item.frequencies.begin(), opp));
        if (opp_index < actual_max || opp_index > actual_min) return false;

        if (!item.mutation_active) {
            const auto audit = store.get_audit_trail();
            if (!snapshot_is_oem(audit, item.maximum, actual_max) ||
                !snapshot_is_oem(audit, item.minimum, actual_min)) return false;
        } else {
            const auto audit = store.get_audit_trail();
            const auto owned_endpoint = [&](const std::string& path, uint64_t value, uint64_t baseline) {
                const auto node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& entry) {
                    return entry.path == path;
                });
                const std::string expected = std::to_string(value);
                return node != audit.end() && node->factory_value == std::to_string(baseline) &&
                    node->active_value == expected && node->is_modified == (value != baseline);
            };
            if (!owned_endpoint(item.maximum, actual_max, item.baseline) ||
                !owned_endpoint(item.minimum, actual_min, item.baseline_minimum)) return false;
        }
        sample.current_frequency_hz = actual_frequency;
        sample.max_pwrlevel = actual_max;
        sample.min_pwrlevel = actual_min;
    }
    return observed_domains == 1U;
}

bool FrequencyController::gpu_frequency_observation_ready() const noexcept {
    GpuFrequencySample sample{};
    return read_gpu_frequency_sample(sample);
}
void FrequencyController::discover(bool cpu, bool gpu) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Never discard in-memory ownership metadata while a mutation is active.
    if (std::any_of(domains_.begin(), domains_.end(), [](const Domain& d) { return d.mutation_active; })) return;
    domains_.clear();

    if (profile_) {
        const auto eligible = [&](const std::string& path, std::string_view semantic) {
            return std::any_of(profile_->node_capabilities.begin(), profile_->node_capabilities.end(),
                [&](const NodeCapability& cap) {
                    return cap.path == path && cap.semantic_id == semantic && cap.mutation_eligible();
                });
        };
        if (cpu) {
            for (const auto& policy : profile_->cpu_policies) {
                const std::string max_path = policy.policy_path + "/scaling_max_freq";
                if (eligible(max_path, "cpufreq.scaling_max_freq")) add(policy.policy_path, false);
            }
        }
        if (gpu) {
            for (const auto& domain : profile_->gpu_domains) {
                if (domain.device_path.find("/class/kgsl/") != std::string::npos) {
                    add_kgsl(domain.device_path);
                    continue;
                }
                const std::string max_path = domain.device_path + "/max_freq";
                if (eligible(max_path, "devfreq.max_freq")) add(domain.device_path, true);
            }
            if (std::none_of(profile_->gpu_domains.begin(), profile_->gpu_domains.end(),
                    [](const GpuDomainProfile& domain) {
                        return domain.device_path.find("/class/kgsl/") != std::string::npos;
                    })) add_kgsl(root_ + "/class/kgsl/kgsl-3d0");
        }
        // An injected profile is authoritative for CPU/GPU actuator admission.
        // Do not run a second independent scan when a subsystem was not admitted.
        return;
    }

    // Legacy compatibility path for tests/callers that have not yet injected CDDP.
    auto scan = [&](const std::string& root, bool is_gpu) {
        std::error_code ec;
        std::set<std::string> seen;
        for (fs::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
            if (domains_.size() >= 32) break;
            const auto name = it->path().filename().string();
            auto real = fs::canonical(it->path(), ec);
            if (ec) { ec.clear(); continue; }
            if (!seen.insert(real.string()).second) continue;
            if (is_gpu) {
                std::string device_name;
                std::ifstream(it->path() / "name") >> device_name;
                const auto identity = name + device_name + real.string();
                if (identity.find("gpu") == std::string::npos && identity.find("mali") == std::string::npos &&
                    identity.find("kgsl") == std::string::npos) continue;
            } else if (name.rfind("policy", 0) != 0) continue;
            add(it->path().string(), is_gpu);
        }
    };
    if (cpu) scan(root_ + "/devices/system/cpu/cpufreq", false);
    if (gpu) {
        scan(root_ + "/class/devfreq", true);
        add_kgsl(root_ + "/class/kgsl/kgsl-3d0");
    }
}
bool FrequencyController::apply(double scale, bool thermal_guard) {
    // The generic Rust capacity budget is a CPU/system capacity signal, not a
    // GPU-specific performance objective. Never route it into KGSL power-level
    // writes. Qualcomm's paired actuator is deliberately callable only through
    // apply_gpu_frame_policy(), which requires fresh measured GPU + frame evidence
    // and thermal headroom. This prevents a second, unmeasured GPU authority.
    // 0.2 is the emergency thermal floor; hardware policy floors still apply per domain.
    if (!std::isfinite(scale) || scale < 0.2 || scale > 1.0) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (thermal_guard && !rollback_domain(true)) return false;
    auto& store = BaselineStore::instance();
    bool ok = true;
    const auto restore_owned_domain = [&](Domain& domain) {
        if (!store.rollback(domain.maximum)) return false;
        uint64_t restored = 0;
        if (!number(domain.maximum, restored) || restored != domain.baseline) return false;
        domain.last = domain.baseline;
        domain.mutation_active = false;
        return true;
    };
    bool cpu_needs_lease = false;
    bool gpu_needs_lease = false;

    // First determine whether each domain actually needs a write, then acquire the
    // corresponding exclusive lease before the first mutation in that domain.
    for (auto& domain : domains_) {
        // Paired KGSL power levels must never be driven by the generic capacity scale.
        if (domain.retired || domain.kgsl_power_levels) continue;
        uint64_t actual = 0, floor = 0;
        if (!number(domain.maximum, actual) || !number(domain.minimum, floor)) {
            domain.retired = true;
            ok = false;
            continue;
        }
        const auto ownership_audit = store.get_audit_trail();
        const auto ownership_snapshot = std::find_if(
            ownership_audit.begin(), ownership_audit.end(), [&](const NodeSnapshot& node) {
                return node.path == domain.maximum;
            });
        if (ownership_snapshot == ownership_audit.end() ||
            (ownership_snapshot->is_modified && !domain.mutation_active)) {
            // A journaled mutation appeared after discovery but is not owned by
            // this controller instance. Retire instead of adopting its active value.
            domain.retired = true;
            ok = false;
            continue;
        }
        if (actual != domain.last) {
            // An external owner changed the node. Do not overwrite its value.
            domain.retired = true;
            if (domain.mutation_active) {
                // BaselineStore's ownership check prevents rollback from clobbering
                // an external value; retain the lease if safe restoration is unproven.
                if (!store.rollback(domain.maximum)) ok = false;
                else domain.mutation_active = false;
            }
            continue;
        }
        const auto requested = static_cast<uint64_t>(static_cast<double>(domain.baseline) * scale);
        auto upper = std::upper_bound(domain.frequencies.begin(), domain.frequencies.end(), requested);
        uint64_t target = upper == domain.frequencies.begin() ? *upper : *std::prev(upper);
        if (target < floor) {
            const auto next = std::lower_bound(domain.frequencies.begin(), domain.frequencies.end(), floor);
            if (next == domain.frequencies.end()) {
                domain.retired = true;
                if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
                continue;
            }
            target = *next;
        }
        if (scale == 1.0) target = domain.baseline;
        if (target < floor || target > domain.baseline) {
            domain.retired = true;
            if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
            continue;
        }
        if (target == actual) continue;
        if (domain.gpu) gpu_needs_lease = true;
        else cpu_needs_lease = true;
    }

    if (cpu_needs_lease && !acquire_domain_lease(false, thermal_guard)) return false;
    if (gpu_needs_lease && !acquire_domain_lease(true, thermal_guard)) {
        if (!cpu_transaction_id_.empty()) {
            const bool cpu_active = std::any_of(domains_.begin(), domains_.end(),
                [](const Domain& d) { return !d.gpu && d.mutation_active; });
            if (!cpu_active) release_domain_lease(false);
        }
        return false;
    }

    for (auto& domain : domains_) {
        if (domain.retired || domain.kgsl_power_levels) continue;
        uint64_t actual = 0, floor = 0;
        if (!number(domain.maximum, actual) || !number(domain.minimum, floor) || actual != domain.last) {
            domain.retired = true;
            ok = false;
            continue;
        }
        const auto requested = static_cast<uint64_t>(static_cast<double>(domain.baseline) * scale);
        auto upper = std::upper_bound(domain.frequencies.begin(), domain.frequencies.end(), requested);
        uint64_t target = upper == domain.frequencies.begin() ? *upper : *std::prev(upper);
        if (target < floor) {
            const auto next = std::lower_bound(domain.frequencies.begin(), domain.frequencies.end(), floor);
            if (next == domain.frequencies.end()) {
                domain.retired = true;
                if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
                continue;
            }
            target = *next;
        }
        if (scale == 1.0) target = domain.baseline;
        if (target < floor || target > domain.baseline) {
            domain.retired = true;
            if (domain.mutation_active && !restore_owned_domain(domain)) ok = false;
            continue;
        }
        if (target == actual) continue;

        if (target == domain.baseline) {
            if (!store.rollback(domain.maximum)) {
                ok = false;
                continue;
            }
            uint64_t restored = 0;
            if (!number(domain.maximum, restored) || restored != domain.baseline) {
                ok = false;
                continue;
            }
            domain.last = domain.baseline;
            domain.mutation_active = false;
            continue;
        }

        const bool applied = store.apply_mutation(domain.maximum, std::to_string(target));
        uint64_t readback = 0;
        const bool readback_valid = number(domain.maximum, readback);
        const auto audit = store.get_audit_trail();
        const auto snapshot = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == domain.maximum;
        });
        const bool store_still_owns_node =
            snapshot != audit.end() && snapshot->is_modified &&
            store.owns_expected_state(domain.maximum, snapshot->active_value);

        if (!applied || !readback_valid || readback != target) {
            ok = false;
            if (!readback_valid) {
                // The node became unreadable after a write attempt. Retain the lease
                // and recovery metadata regardless of the journal's last visible state.
                domain.mutation_active = true;
                continue;
            }

            if (store_still_owns_node) {
                // A failed apply can leave either the prior CoreFlow value or a
                // partially committed target. Restore this controller's path before
                // considering its lease releasable.
                domain.mutation_active = true;
                domain.last = readback;
                if (!restore_owned_domain(domain)) {
                    const auto after_audit = store.get_audit_trail();
                    const auto after_snapshot = std::find_if(
                        after_audit.begin(), after_audit.end(), [&](const NodeSnapshot& node) {
                            return node.path == domain.maximum;
                        });
                    if (after_snapshot == after_audit.end() || !after_snapshot->is_modified) {
                        // BaselineStore adopted an OEM/external value during rollback.
                        // Relinquish and retire instead of reasserting on a later tick.
                        domain.mutation_active = false;
                        domain.retired = true;
                        domain.last = readback;
                    }
                    continue;
                }
            } else {
                // BaselineStore no longer recognizes this as our mutation. Preserve
                // an OEM/external value and retire the path instead of reasserting.
                domain.mutation_active = false;
                domain.last = readback;
                if (readback != domain.baseline) domain.retired = true;
            }
            continue;
        }

        domain.mutation_active = true;
        domain.last = target;
    }

    for (const bool gpu : {false, true}) {
        const bool active = std::any_of(domains_.begin(), domains_.end(),
            [gpu](const Domain& d) { return d.gpu == gpu && d.mutation_active; });
        if (!active) release_domain_lease(gpu);
    }
    return ok;
}
size_t FrequencyController::cpu_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(domains_.begin(), domains_.end(), [](const auto& d) { return !d.gpu && !d.retired; });
}
size_t FrequencyController::gpu_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(domains_.begin(), domains_.end(), [](const auto& d) { return d.gpu && !d.retired; });
}
size_t FrequencyController::kgsl_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(domains_.begin(), domains_.end(),
        [](const Domain& d) { return d.gpu && d.kgsl_power_levels && !d.retired; });
}
}
`,a0=`#include "coreflow/power_bridge.hpp"
#include <unistd.h>
#ifdef __ANDROID__
#include <dlfcn.h>
struct AIBinder;
#endif

namespace coreflow {
PowerCapabilities PowerBridge::probe() const {
    PowerCapabilities result;
    result.binder_device = ::access("/dev/binder", R_OK | W_OK) == 0;
    result.hwbinder_device = ::access("/dev/hwbinder", R_OK | W_OK) == 0;
#ifdef __ANDROID__
    // Service-manager interfaces are platform APIs, not public NDK headers.
    // Probe optional exported C symbols without depending on private headers or
    // vendor-generated AIDL stubs. Keep Binder loaded for the process lifetime.
    static void* library = ::dlopen("libbinder_ndk.so", RTLD_NOW | RTLD_LOCAL);
    if (library) {
        using Check = AIBinder* (*)(const char*);
        using Release = void (*)(AIBinder*);
        const auto check = reinterpret_cast<Check>(::dlsym(library, "AServiceManager_checkService"));
        const auto release = reinterpret_cast<Release>(::dlsym(library, "AIBinder_decStrong"));
        if (check && release) {
            if (auto* binder = check("android.hardware.power.IPower/default")) {
                result.aidl_power_service = true;
                release(binder);
            }
        }
    }
#endif
    return result;
}
std::expected<void, std::string_view> PowerBridge::enable_interception() const {
    return std::unexpected("Requires an OEM/AOSP service integration; no transparent interception backend is installed");
}
}
`,i0=["Architecture","Mutation Safety","Risk & Provenance","HAL / Binder","Thermal Simulator","C++23 / Rust 2024","Export"],l0=o=>{const[c,m]=Q.useState("Architecture"),[d,j]=Q.useState(75),[O,N]=Q.useState(30),[L,B]=Q.useState(!1),[R,x]=Q.useState(""),[g,q]=Q.useState("Autonomous tuner"),$={"Autonomous tuner":e0,"Neural predictor":t0,"Rust controller":$g,"cxx bridge":Wg,"CPU/GPU controller":n0,"Power HAL probe":a0},M=Q.useMemo(()=>{let C=O,z=0,P=C,ye=1;const Re=[];for(let Me=0;Me<=120;Me++){const Ne=42.5-C;z=Math.max(-50,Math.min(0,z+Ne));const $e=Me===0?0:C-P,it=Math.max(.4,Math.min(1,1+(2.5*Ne+.15*z-1.2*$e)/50));ye+=Math.max(-.02,Math.min(.02,it-ye)),P=C,C+=d/100*ye*.5-(C-O)*.025,Re.push({temperature:C,scale:ye})}return Re},[d,O]),{isOpen:X,onClose:I}=o;if(Q.useEffect(()=>{if(!X)return;const C=z=>{z.key==="Escape"&&I()};return window.addEventListener("keydown",C),()=>window.removeEventListener("keydown",C)},[X,I]),Q.useEffect(()=>{if(!L)return;const C=window.setTimeout(()=>B(!1),2e3);return()=>window.clearTimeout(C)},[L]),!X)return null;const J=Jg+`

## CFAE System Framework v4.0.0
## Current Hardware
SoC: ${o.socModel}
Kernel: ${o.kernelVersion}
CPUs: ${o.onlineCpus}
GPU: ${o.gpuType}
`,ae=async()=>{try{await navigator.clipboard.writeText(J),B(!0),x("")}catch{x("Clipboard unavailable. Use Download.")}},F=C=>{const z=URL.createObjectURL(new Blob([J],{type:"text/plain;charset=utf-8"})),P=document.createElement("a");P.href=z,P.download=`cfae_framework_blueprint.${C}`,document.body.appendChild(P),P.click(),P.remove(),window.setTimeout(()=>URL.revokeObjectURL(z),1e3)},re=M.map((C,z)=>`${z*5},${200-(C.temperature-20)*4}`).join(" "),xe=M.map((C,z)=>`${z*5},${200-C.scale*160}`).join(" ");return r.jsx("div",{className:"fixed inset-0 z-50 flex items-center justify-center p-3 sm:p-6 backdrop-blur-xl md3-page-stage",style:{background:"color-mix(in srgb, var(--md-sys-color-scrim) 55%, transparent)"},children:r.jsxs("div",{role:"dialog","aria-modal":"true","aria-label":"CFAE System Framework Architecture Studio",className:"relative w-full max-w-5xl h-[88vh] flex flex-col md3-card-elevated overflow-hidden",style:{background:"var(--md-sys-color-surface-container-high)",border:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("header",{className:"px-6 py-4 flex justify-between items-center gap-3",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-center gap-3",children:[r.jsx("div",{className:"w-10 h-10 rounded-2xl flex items-center justify-center shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(su,{size:19})}),r.jsxs("div",{children:[r.jsx("h2",{className:"font-semibold text-base sm:text-lg tracking-tight",style:{color:"var(--md-sys-color-on-surface)"},children:"CFAE Architecture & Mutation Safety Studio"}),r.jsx("p",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-primary)"},children:"v4.0.0 · C++23 + Rust 2024 + Kotlin · Coordinated Mutation Transactions"})]})]}),r.jsx("button",{onClick:o.onClose,"aria-label":"Close Studio",className:"md3-icon-btn",children:r.jsx(sp,{size:18})})]}),r.jsx("nav",{className:"flex flex-wrap gap-1.5 px-6 py-3",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)",background:"var(--md-sys-color-surface-container)"},"aria-label":"Studio sections",children:i0.map(C=>r.jsx("button",{onClick:()=>m(C),className:"md3-chip "+(C===c?"md3-chip-active":""),children:C},C))}),r.jsxs("div",{className:"flex-1 min-h-0 overflow-auto p-6 sm:p-8 space-y-6 text-sm",children:[c==="Architecture"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(Aa,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Multi-Language Native Architecture"]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Sensor & Capability Discovery → State Normalization → Reasoning Engine → Dependency Analyzer → Mutation Planner → Safety Admission Gate → Transaction Coordinator (Apply Order + Ownership Guard) → Readback Verify → Outcome Evaluator → Retain / Rollback → Outcome Learning."})]}),r.jsx("div",{className:"grid sm:grid-cols-2 gap-4",children:[["ISO C++23 Core Runtime","Zero-polling epoll/timerfd/signalfd loop, semantic cgroup v2 & sysfs discovery, O_NOFOLLOW + fstat identity verification, and fsync write-ahead journal."],["Rust 2024 Decision Engine","Memory-safe PID thermal controller, hysteresis state machine, PSI contention arbitration, and bounded capacity scaling via cxx bridge."],["Kotlin 2.1 Android Bridge","Android 15 (API 35) Quick Settings TileService and Battery thermal broadcast integration."],["Material 3 Expressive WebUI","Offline KernelSU/APatch workbench displaying live runtime status or clearly labeled preview telemetry with dynamic seed palettes."]].map(([C,z])=>r.jsxs("div",{className:"md3-card-filled p-5 space-y-1.5",children:[r.jsx("h4",{className:"text-xs sm:text-sm font-semibold",style:{color:"var(--md-sys-color-primary)"},children:C}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:z})]},C))}),r.jsxs("div",{className:"md3-card-outlined p-5 font-mono text-xs space-y-1.5",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("div",{children:["SoC: ",o.socModel]}),r.jsxs("div",{children:["Kernel: ",o.kernelVersion]}),r.jsxs("div",{children:["Online CPUs: ",o.onlineCpus]}),r.jsxs("div",{children:["GPU: ",o.gpuType]})]})]}),c==="Mutation Safety"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(On,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Coordinated Optimization & 5-Layer Mutation Safety"]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["The primary unit of control is the ",r.jsx("strong",{children:"Mutation Transaction"}),": one explicit optimization goal, a dependency-verified change plan within a bounded risk budget, per-write readback verification, and ownership-safe rollback."]})]}),r.jsx("div",{className:"grid sm:grid-cols-2 lg:grid-cols-3 gap-4",children:[["1. Reasoning Engine","Synthesizes workload observations, CPU/Memory/I/O PSI, thermal headroom, and power state into an inspectable optimization intent."],["2. Dependency Analyzer","Builds a provenance-backed dependency graph (Required-before, Requires, Coupled-with, Conflicts-with, Unknown). Unknown edges are never grouped."],["3. Mutation Planner","Constructs a bounded transaction plan specifying baseline, target, allowed range source, execution order, risk budget, and abort conditions."],["4. Safety Admission Gate","Verifies 10 pre-write invariants: object identity, semantic units, valid baseline, zero ownership conflict, and OEM thermal compliance."],["5. Coordinator & Readback","Serializes conflicting transactions, writes the durable journal prior to mutation, applies changes in dependency order, and verifies readback."],["6. Outcome Learning","Evaluates Stability, Responsiveness, Pressure, Thermal, and Power outcomes (IMPROVED, NEUTRAL, REGRESSED, OWNERSHIP_LOST) with anti-repeat deferral."]].map(([C,z])=>r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsx("h4",{className:"text-xs font-semibold",style:{color:"var(--md-sys-color-primary)"},children:C}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:z})]},C))}),r.jsxs("div",{className:"md3-card-outlined p-5 text-xs space-y-2",children:[r.jsx("div",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"12 Non-Negotiable Safety Invariants:"}),r.jsx("p",{className:"leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"1. Zero write without valid baseline · 2. Zero mutation without known semantics & range · 3. Zero grouped mutation without verified relationship · 4. No concurrent conflicting transactions · 5. Mandatory readback after every write · 6. Rollback never overwrites external OEM/Android changes · 7. No blind retries without new evidence · 8. Retention requires valid outcome & ownership · 9. No unverified metric claims · 10. OEM/Android thermal policy remains supreme · 11. Write-ahead journal & idempotent recovery · 12. Fail-safe default is no write."})]})]}),c==="Risk & Provenance"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsx("h3",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Multi-Dimensional Risk Model & Dependency Provenance"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CoreFlow evaluates seven distinct risk dimensions rather than a single opaque score: Semantic uncertainty, Platform uncertainty, Interaction risk, Reversibility, System impact, Observability, and Ownership confidence."})]}),r.jsx("div",{className:"grid sm:grid-cols-2 gap-4",children:[["Low Risk Class","Strong semantics, bounded impact, reversible, zero conflict, high observability. Eligible for coupled mutation groups within budget."],["Medium Risk Class","Limited interaction or platform uncertainty with understood dependencies and verified rollback. Applied step-by-step in one transaction."],["High Risk Class","Broad impact, uncertain platform semantics, or weak observability. Strictly isolated or rejected; never grouped with other changes."],["Forbidden Class","Missing baseline, unsafe rollback path, ownership conflict, or violates OEM thermal/power safety policy. Never mutated."]].map(([C,z])=>r.jsxs("div",{className:"md3-card-filled p-4 space-y-1",children:[r.jsx("div",{className:"text-xs font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:C}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:z})]},C))}),r.jsxs("div",{className:"md3-card-outlined p-5 space-y-2 text-xs",children:[r.jsx("div",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"Transaction State Machine Protocol"}),r.jsx("div",{className:"font-mono leading-relaxed",style:{color:"var(--md-sys-color-primary)"},children:"PROPOSED → ADMISSION_CHECK → PREPARED → APPLYING → READBACK_VERIFY → OBSERVING → COMMITTED → RETAINED"}),r.jsx("p",{className:"leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Failed admission transitions to DEFERRED / REJECTED. Readback mismatch or observed regression transitions immediately to ROLLING_BACK under Ownership Guard verification."})]})]}),c==="HAL / Binder"&&r.jsxs(r.Fragment,{children:[r.jsx("h3",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Platform Integration & OEM Boundaries"}),r.jsx("div",{className:"md3-card-filled overflow-hidden",children:r.jsxs("table",{className:"w-full text-xs text-left",children:[r.jsx("thead",{children:r.jsxs("tr",{style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("th",{className:"p-4 font-semibold",children:"Capability"}),r.jsx("th",{className:"p-4 font-semibold",children:"Implementation Status"})]})}),r.jsx("tbody",{children:[["AIDL Power HAL discovery","Nonblocking libbinder_ndk lookup; capability detection only."],["Qualcomm / MediaTek interception","Unavailable: requires a registered, versioned OEM/AOSP proxy backend."],["Binder device access","Does not expose or reroute other processes’ transactions."],["NPU / APU offload","Unavailable: requires a compatible compute workload and runtime."]].map(([C,z])=>r.jsxs("tr",{style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("td",{className:"p-4 font-medium",style:{color:"var(--md-sys-color-primary)"},children:C}),r.jsx("td",{className:"p-4",style:{color:"var(--md-sys-color-on-surface-variant)"},children:z})]},C))})]})}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"OEM thermal services remain authoritative. The daemon never replaces Power HAL services, patches Binder, or injects undocumented vendor transactions."})]}),c==="Thermal Simulator"&&r.jsxs(r.Fragment,{children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(ra,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Illustrative Thermal Equilibrium Plant"]}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Synthetic first-order thermal plant for previewing PID slew bounding. Separate from native device sensor telemetry."}),r.jsxs("div",{className:"grid sm:grid-cols-2 gap-4",children:[r.jsxs("label",{className:"md3-card-filled p-4 block text-xs",children:[r.jsxs("span",{className:"font-medium",style:{color:"var(--md-sys-color-on-surface)"},children:["Workload Intensity: ",d,"%"]}),r.jsx("input",{"aria-label":"Simulated workload",className:"block w-full mt-2",type:"range",min:"10",max:"100",value:d,onChange:C=>j(Number(C.target.value))})]}),r.jsxs("label",{className:"md3-card-filled p-4 block text-xs",children:[r.jsxs("span",{className:"font-medium",style:{color:"var(--md-sys-color-on-surface)"},children:["Ambient Temperature: ",O," °C"]}),r.jsx("input",{"aria-label":"Simulated ambient temperature",className:"block w-full mt-2",type:"range",min:"20",max:"40",value:O,onChange:C=>N(Number(C.target.value))})]})]}),r.jsxs("svg",{viewBox:"0 0 620 235",role:"img","aria-label":"Temperature and capacity scale over 120 simulated seconds",className:"w-full md3-card-filled p-2",children:[r.jsx("line",{x1:"10",y1:"200",x2:"610",y2:"200",stroke:"#64748b",strokeOpacity:"0.4"}),r.jsx("polyline",{points:re,fill:"none",stroke:"#f59e0b",strokeWidth:"2.2",transform:"translate(10,0)"}),r.jsx("polyline",{points:xe,fill:"none",stroke:"var(--md-sys-color-primary)",strokeWidth:"2.2",transform:"translate(10,0)"}),r.jsx("text",{x:"12",y:"222",fill:"#64748b",fontSize:"11",children:"0 s"}),r.jsx("text",{x:"565",y:"222",fill:"#64748b",fontSize:"11",children:"120 s"})]}),r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs font-mono",children:[r.jsxs("span",{style:{color:"var(--md-sys-color-on-surface)"},children:["Final: ",M[120].temperature.toFixed(1)," °C ·"," ",(M[120].scale*100).toFixed(0),"% capacity budget"]}),r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Amber: Temperature (20–70 °C) · Accent: Capacity Scale (40–100%)"})]})]}),c==="C++23 / Rust 2024"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"flex items-center justify-between gap-3 flex-wrap",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(np,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Production Source Inspector"]}),r.jsx("select",{"aria-label":"Source file",value:g,onChange:C=>q(C.target.value),className:"md3-btn-tonal text-xs",children:Object.keys($).map(C=>r.jsx("option",{children:C},C))})]}),r.jsx("pre",{className:"overflow-auto text-xs font-mono p-5 md3-card-filled whitespace-pre max-h-96 leading-relaxed",style:{color:"var(--md-sys-color-on-surface)"},children:$[g]})]}),c==="Export"&&r.jsxs(r.Fragment,{children:[r.jsx("h3",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Specification & Blueprint Export"}),r.jsxs("div",{className:"flex gap-2.5 flex-wrap",children:[r.jsxs("button",{className:"md3-btn-filled",onClick:()=>void ae(),children:[r.jsx(lu,{size:14}),L?"Copied":"Copy Blueprint"]}),["md","txt"].map(C=>r.jsxs("button",{className:"md3-btn-outlined",onClick:()=>F(C),children:[r.jsx(Qh,{size:14}),"Download .",C]},C))]}),R&&r.jsx("p",{role:"alert",className:"text-xs",style:{color:"var(--md-sys-color-warning)"},children:R}),r.jsx("pre",{className:"whitespace-pre-wrap font-mono text-xs md3-card-filled p-5 max-h-96 overflow-auto leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:J})]})]})]})})},r0=({telemetry:o,nativeMode:c,thermalTripCelsius:m})=>{const[d,j]=Q.useState(null),[O,N]=Q.useState(""),L=o.socCompositeCelsius&&o.socCompositeCelsius>0?o.socCompositeCelsius:o.socTempCelsius,B=o.cpuAverageCelsius??-1,R=o.cpuPeakCelsius??-1,x=o.gpuTempCelsius??-1,g=o.batteryTempCelsius??-1,q=o.skinTempCelsius??-1,$=c?!!o.thermalGuardActive:L>=m,M=Math.min(100,Math.max(0,(L-30)/30*100)),X=o.thermalSensors??[],I=X.filter(F=>!(d!==null&&F.category!==d||O&&!F.type.toLowerCase().includes(O.toLowerCase()))),J=F=>{switch(F){case 1:return"Compute · CPU/GPU";case 2:return"Battery Cell";case 3:return"Skin · Chassis";case 4:return"Board · PMIC";default:return"Diagnostic · Aux"}},ae=F=>{const re=F.trim();if(re.startsWith("/sys/class/thermal/thermal_zone"))return`tz${re.slice(31)}`;if(re.startsWith("thermal_zone"))return`tz${re.slice(12)}`;const xe=re.split("/").filter(Boolean);return(xe[xe.length-1]??re).replace(/^thermal_zone/,"tz")};return r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx(ns,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("h2",{className:"font-bold text-base sm:text-lg",children:["Multi-Sensor Thermal & OEM Policy Matrix",c?"":" (sim)"]}),r.jsx("span",{className:$?"md3-badge md3-badge-warning":"md3-badge md3-badge-success",children:$?"ThermalGuard Active Mitigation":"Thermal Headroom Nominal"}),o.thermalDataDegraded&&r.jsx("span",{className:"md3-badge md3-badge-warning",children:"Sensor Degraded (Partial Read)"}),o.thermalFallbackActive&&r.jsx("span",{className:"md3-badge md3-badge-warning",children:"Generic Fallback Mode"})]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Prioritizes validated Android NDK thermal severity and per-zone OEM trip points over single-sensor heuristics. Compute, battery, skin, and PMIC/board domains are evaluated independently with physical velocity tracking."})]}),r.jsxs("div",{className:"md3-card-filled px-4 py-2.5 text-xs font-mono shrink-0 self-start",children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Policy Reference: "}),r.jsx("span",{className:"font-bold",children:c?o.thermalFallbackActive?`Fallback (${m.toFixed(1)} °C)`:`OEM / Android (${o.thermalSource??"HAL"})`:`${m.toFixed(1)} °C`})]})]}),r.jsxs("div",{className:"md3-card-filled p-5 space-y-3",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",children:[r.jsxs("span",{className:"flex items-center gap-2 font-bold",children:[r.jsx(rp,{className:"w-4 h-4 shrink-0",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Weighted SoC Composite Diagnostic",c?"":" (sim)"]})]}),r.jsxs("span",{className:"font-mono tabular-nums font-bold text-base shrink-0",children:[L>0?L.toFixed(1):"--.-"," °C"]})]}),r.jsx("div",{className:"md3-progress-track",children:r.jsx("div",{className:"md3-progress-bar",style:{width:`${M}%`}})}),r.jsxs("div",{className:"flex justify-between text-[11px] font-mono tabular-nums",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"30.0 °C (Idle)"}),r.jsxs("span",{children:[m.toFixed(1)," °C (Mitigation Gate)"]}),r.jsx("span",{children:"60.0 °C (OEM Ceiling)"})]})]}),r.jsx("div",{className:"grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 gap-3",children:[{label:"CPU Cluster Avg",val:B,Icon:Aa,sub:"All compute cores"},{label:"CPU Peak Core",val:R,Icon:ns,sub:"Hottest core die"},{label:"GPU Domain",val:x,Icon:ra,sub:"Graphics subsystem"},{label:"Battery Cell",val:g,Icon:Ah,sub:"Charging protection"},{label:"Skin / Chassis",val:q,Icon:xg,sub:"Surface comfort"}].map(({label:F,val:re,Icon:xe,sub:C})=>r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs mb-2",children:[r.jsx("span",{className:"font-semibold truncate",children:F}),r.jsx(xe,{className:"w-3.5 h-3.5 shrink-0",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"text-xl font-bold font-mono tabular-nums",children:[re>0?re.toFixed(1):"--.-"," ",r.jsx("span",{className:"text-xs font-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"°C"})]}),r.jsx("div",{className:"text-[11px] mt-1.5",style:{color:"var(--md-sys-color-on-surface-variant)"},children:C})]},F))}),X.length>0&&r.jsxs("details",{className:"group md3-card-filled overflow-hidden",open:!0,children:[r.jsxs("summary",{className:"cursor-pointer p-4 sm:p-5 flex items-center justify-between text-xs sm:text-sm font-bold select-none",children:[r.jsxs("div",{className:"flex items-center gap-2.5 min-w-0",children:[r.jsx(On,{className:"w-4 h-4 shrink-0",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{className:"truncate",children:["Discovered Hardware Thermal Zones",c?"":" (sim)"," (",X.length," Zones)"]})]}),r.jsx(Rh,{className:"w-4 h-4 shrink-0 transition-transform duration-200 group-open:rotate-180"})]}),r.jsxs("div",{className:"p-4 sm:p-5 pt-0 space-y-4",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-center justify-between gap-3 pt-4",children:[r.jsx("div",{className:"flex flex-wrap gap-1.5",children:[{id:null,label:"All Domains"},{id:1,label:"Compute"},{id:2,label:"Battery"},{id:3,label:"Skin"},{id:4,label:"Board/PMIC"}].map(F=>{const re=d===F.id;return r.jsx("button",{type:"button","aria-pressed":re,onClick:()=>j(F.id),className:"md3-chip "+(re?"is-selected":""),children:F.label},String(F.id))})}),r.jsxs("div",{className:"relative",children:[r.jsx(lp,{className:"w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none",style:{color:"var(--md-sys-color-on-surface-variant)"}}),r.jsx("input",{type:"text","aria-label":"Search thermal zone",placeholder:"Search zone...",value:O,onChange:F=>N(F.target.value),className:"w-full sm:w-52 pl-9 pr-3.5 py-1.5 text-xs rounded-full focus:outline-none",style:{backgroundColor:"var(--md-sys-color-surface-container-lowest)",border:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface)"}})]})]}),r.jsx("div",{className:"grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-2.5 max-h-80 overflow-y-auto pr-1",children:I.length===0?r.jsx("div",{className:"col-span-full text-center py-6 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No thermal sensors match your filter."}):I.map(F=>r.jsxs("div",{className:"md3-card-elevated p-3.5 flex items-center justify-between gap-2",children:[r.jsxs("div",{className:"min-w-0 pr-2",children:[r.jsx("div",{className:"text-xs font-mono font-bold truncate",title:F.type,children:F.type}),r.jsxs("div",{className:"text-[11px] truncate",style:{color:"var(--md-sys-color-on-surface-variant)"},title:F.id,children:[ae(F.id)," · ",J(F.category)]})]}),r.jsxs("div",{className:"text-right shrink-0",children:[r.jsx("div",{className:"text-xs font-mono tabular-nums font-bold",children:F.valid&&F.celsius!==null?`${F.celsius.toFixed(1)} °C`:r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"N/A"})}),r.jsx("div",{className:"text-[10px] font-mono tabular-nums",style:{color:"var(--md-sys-color-on-surface-variant)"},children:F.velocityValid&&F.velocityCelsiusPerSec!==null&&F.velocityCelsiusPerSec!==void 0?`${F.velocityCelsiusPerSec>0?"+":""}${F.velocityCelsiusPerSec.toFixed(2)} °C/s`:F.policyAvailable?"OEM trip":"no-trip"})]})]},F.id))})]})]})]})},s0=({nativeMode:o,onRefreshTelemetry:c})=>{const[m,d]=Q.useState("Select an inspection action above to query coreflowd runtime JSON, capabilities, recovery journal, or CF-DVS state."),[j,O]=Q.useState("Ready"),[N,L]=Q.useState(!1),[B,R]=Q.useState(!1),x=Q.useRef(null);Q.useEffect(()=>()=>{x.current!==null&&window.clearTimeout(x.current)},[]);const g='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"',q=X=>{const I=X.trim(),J=I.split(`
`).map(re=>re.trim()).filter(Boolean);for(let re=J.length-1;re>=0;re--)if(J[re].startsWith("{")&&J[re].endsWith("}"))try{return JSON.parse(J[re])}catch{}const ae=I.indexOf("{"),F=I.lastIndexOf("}");return ae>=0&&F>ae?JSON.parse(I.slice(ae,F+1).replace(/\r?\n/g,"")):JSON.parse(J[J.length-1]??"{}")},$=async(X,I)=>{L(!0),O(I);try{if(!ls()){X.includes("--status")?d(JSON.stringify({running:!0,pid:24108,uclampMin:320,uclampMax:1024,uclampSupported:!0,platformVendor:"Universal Android",osFamily:"Android AOSP / GKI",executionTier:"UniversalFallback",policyFamily:"CoreFlowBase",cpuPressurePct:1.2,memoryPressurePct:.3,ioPressurePct:.4,baselineNodesSnapshotted:6,socTempCelsius:38.6,socCompositeCelsius:38.6,cpuAverageCelsius:38.2,cpuPeakCelsius:41.5,gpuTempCelsius:36.8,batteryTempCelsius:33.4,skinTempCelsius:35.1,recoveryPending:!1,thermalPolicyAvailable:!0,thermalSource:"multi-sensor-composite",thermalSensor:"SoC-Composite (8 CPU cores, 1 GPU domain)",thermalSeverity:0,thermalPressure:44,aiBackend:"embedded-rls-mlp",aiOnnxActive:!1,aiPredictedPressure10s:46.2,aiPredictedVelocity:.08,aiRecommendedScale:.94,aiConfidence:.86,tunerStage:"RETAINED",tunerIntent:"HoldEquilibrium",tunerCandidateId:"io.mq_deadline_low_latency",tunerVerdict:"IMPROVED"},null,2)):X.includes("--capabilities")?d(JSON.stringify({socModel:"Preview SoC Profile",kernelVersion:"Linux 6.1-android15-gki",onlineCpus:"0-7",gpuType:"Universal Multi-Vendor GPU",rustEngine:!0,powerHalDetected:!0,aiBackend:"embedded-rls-mlp",onnxRuntimeLoaded:!1,binderInterception:!1,npuOffload:!1,frameTelemetry:!1},null,2)):X.includes("--journal")?d(JSON.stringify({format:"CFJ4",bootId:"preview-boot-epoch-v4",recoveryPending:!1,entries:[]},null,2)):X.includes("--dvs-status")?d("OK INACTIVE (Preview Simulation)"):d(`[${I}]
Status: OK (Preview Sandbox)
Exit code: 0`);return}const J=await ji(X),ae=J.errno!==0?`[exit ${J.errno}] ${J.stderr.trim()||"Command failed"}
`:"";if(!J.stdout.trim())d(ae||J.stderr||"Command executed with no output.");else try{const F=q(J.stdout);d(ae+JSON.stringify(F,null,2))}catch{d(ae+J.stdout)}X.includes("--status")&&await c()}catch(J){d(`Error: ${String(J)}`)}finally{L(!1)}},M=async()=>{try{await navigator.clipboard.writeText(m)}catch{return}R(!0),x.current!==null&&window.clearTimeout(x.current),x.current=window.setTimeout(()=>{R(!1),x.current=null},2e3)};return r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-5",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx(uu,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"text-base sm:text-lg font-bold",children:"Runtime RPC & Diagnostic Console"}),r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:"coreflow-api.js"}),r.jsx("span",{className:o?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",children:o?"KernelSU Bridge Active":"Preview Simulator"})]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Inspect raw JSON telemetry, discovered hardware capabilities, CFJ4 journal records, and CF-DVS capture status directly from ",r.jsx("code",{className:"font-mono",children:"coreflowd"}),"."]})]}),r.jsxs("button",{type:"button",id:"btn-copy-console-output",onClick:()=>void M(),className:"md3-btn-outlined self-start shrink-0",children:[B?r.jsx(au,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}):r.jsx(lu,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:B?"Copied":"Copy Output"})]})]}),r.jsx("div",{className:"flex flex-wrap gap-2",children:[{label:"Query Status (--status)",cmd:`${g}; "$BIN" --status`,title:"Daemon Status",Icon:ap},{label:"Capabilities (--capabilities)",cmd:`${g}; "$BIN" --capabilities`,title:"Hardware Capabilities",Icon:np},{label:"Inspect Journal (--journal)",cmd:`${g}; "$BIN" --journal`,title:"Recovery Journal",Icon:cu},{label:"CF-DVS Status (--dvs-status)",cmd:`${g}; "$BIN" --dvs-status`,title:"CF-DVS Session Status",Icon:ra},{label:"Inspect coreflow.conf",cmd:"cat /data/adb/coreflow/coreflow.conf 2>/dev/null",title:"Configuration File",Icon:Si}].map(({label:X,cmd:I,title:J,Icon:ae})=>{const F=j===J;return r.jsxs("button",{type:"button",onClick:()=>void $(I,J),disabled:N,className:"md3-chip "+(F?"is-selected":""),children:[r.jsx(ae,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:X})]},J)})}),r.jsxs("div",{className:"md3-card-filled p-5 font-mono text-xs overflow-x-auto max-h-80",children:[r.jsxs("div",{className:"flex items-center justify-between pb-2.5 mb-2.5 text-[11px]",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["Inspector Output · ",j]}),r.jsx("span",{children:N?"Executing query…":"Read-only RPC"})]}),r.jsx("pre",{className:"whitespace-pre leading-relaxed",children:m})]})]})},o0='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"; [ -f "$BIN" ] || exit 127',c0=[{code:"DVS-01",title:"Boot stability",detail:"Daemon readiness, initial hardware discovery, boot_epoch tracking, and recovery state"},{code:"DVS-02",title:"Idle equilibrium",detail:"Baseline stability, drift detection, and release of owned mutations after sustained quiet windows"},{code:"DVS-03",title:"Interactive workload",detail:"Foreground process membership, CPU pressure, and touch/frame response pacing"},{code:"DVS-04",title:"Sustained workload",detail:"Long-running workload admission, OPP ceiling verification, and thermal response"},{code:"DVS-05",title:"Multitasking",detail:"Concurrent memory pressure, block I/O queue contention, and zram swap activity"},{code:"DVS-06",title:"Thermal recovery",detail:"Cooling velocity trend, OEM thermal trip points, and post-load hysteresis stabilization"},{code:"DVS-07",title:"Ownership conflict",detail:"External OEM writer detection, quarantine cooldown, and safe mutation relinquishment"},{code:"DVS-08",title:"Journal recovery",detail:"CFJ4 write-ahead journal integrity, SHA-256 manifest verification, and idempotent rollback"}],u0=({nativeMode:o})=>{const[c,m]=Q.useState("unavailable"),[d,j]=Q.useState(null),[O,N]=Q.useState("Waiting for native validation status."),[L,B]=Q.useState(null),[R,x]=Q.useState(null),[g,q]=Q.useState(!1),$=Q.useCallback(async()=>{if(!o||!ls()){m("unavailable"),j(null),N("Native validation status is unavailable in browser simulation preview.");return}q(!0);try{const I=await ji(`${o0}; "$BIN" --dvs-status`);if(I.errno!==0)throw new Error(I.stderr||I.stdout||"Validation status query failed");const J=I.stdout.trim(),ae=J.match(/^OK ACTIVE ([0-9a-f-]{36})$/i);if(ae)m("active"),j(ae[1]),N("A native validation capture is active. Its lifecycle remains daemon-owned.");else if(J==="OK INACTIVE")m("idle"),j(null),N("No validation capture is currently active.");else throw new Error(J||"Unexpected CF-DVS status response");x(null),B(new Date().toLocaleTimeString())}catch(I){m("unavailable"),j(null),x(I instanceof Error?I.message:String(I)),N("Unable to read the native validation session state.")}finally{q(!1)}},[o]);Q.useEffect(()=>{let I=!1;const J=async()=>{I||await $()};J();const ae=window.setInterval(()=>void J(),15e3);return()=>{I=!0,window.clearInterval(ae)}},[$]);const M=c==="active"?"Capture Active":c==="idle"?"Idle · Ready":"Unavailable",X=c==="active"?"md3-badge md3-badge-success":c==="idle"?"md3-badge md3-badge-primary":"md3-badge md3-badge-warning";return r.jsxs("div",{className:"md3-card-elevated space-y-6 p-6 sm:p-8",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-start gap-3.5",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(ra,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0 flex-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h2",{className:"text-base sm:text-lg font-bold tracking-tight",children:"CoreFlow Device Validation Suite (CF-DVS)"}),r.jsx("span",{className:"md3-badge md3-badge-neutral",children:"READ ONLY · SCHEMA 1.1"})]}),r.jsx("p",{className:"mt-1 text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CoreFlow validates its own runtime invariants and exports SHA-256 manifest bundles with pinned CDDP device profiles. This surface explains coverage and reports native capture state without starting or stopping test sessions."})]})]}),o&&r.jsxs("button",{type:"button",id:"btn-refresh-dvs-status",disabled:g,onClick:()=>void $(),className:"md3-btn-outlined shrink-0 self-start",children:[r.jsx(cu,{className:"h-3.5 w-3.5 "+(g?"animate-spin":"")}),r.jsx("span",{children:"Check Status"})]})]}),r.jsxs("section",{className:"md3-card-filled flex flex-col gap-3 p-4 sm:p-5 sm:flex-row sm:items-center sm:justify-between",children:[r.jsxs("div",{className:"space-y-1.5",children:[r.jsx("div",{className:"text-[11px] font-bold uppercase tracking-wider",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Native CF-DVS Session State"}),r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("span",{className:X,children:M}),d&&r.jsx("span",{className:"font-mono text-xs font-semibold break-all",children:d})]}),r.jsx("p",{className:"break-all text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:O}),L&&r.jsxs("p",{className:"text-[11px] font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Last checked ",L," · auto-refresh every 15s"]})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2 text-xs font-semibold",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(ou,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("span",{children:"Daemon-owned lifecycle"})]})]}),R&&r.jsxs("div",{role:"status",className:"flex items-start gap-2.5 rounded-2xl p-3.5 text-xs",style:{backgroundColor:"var(--md-sys-color-warning-container)",color:"var(--md-sys-color-on-warning-container)"},children:[r.jsx(Si,{className:"mt-0.5 h-4 w-4 shrink-0"}),r.jsx("span",{children:R})]}),r.jsxs("section",{className:"space-y-3",children:[r.jsxs("div",{children:[r.jsx("h3",{className:"text-sm font-bold",children:"8-Domain Evidence & Invariant Matrix"}),r.jsxs("p",{className:"mt-0.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Evidence domains evaluated against the offline Python contract validator (",r.jsx("code",{className:"font-mono",children:"cf_dvs_validate.py"}),")."]})]}),r.jsx("div",{className:"grid grid-cols-1 gap-2.5 sm:grid-cols-2",children:c0.map(I=>r.jsxs("div",{className:"md3-card-outlined flex items-start gap-3 p-3.5",children:[r.jsx(iu,{className:"mt-0.5 h-4 w-4 shrink-0",style:{color:"var(--md-sys-color-success)"}}),r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx("span",{className:"font-mono text-[10px] font-bold",style:{color:"var(--md-sys-color-primary)"},children:I.code}),r.jsx("span",{className:"text-xs font-bold",children:I.title})]}),r.jsx("p",{className:"mt-1 text-[11px] leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:I.detail})]})]},I.title))})]}),r.jsxs("p",{className:"pt-4 text-[11px] leading-relaxed",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:["Diagnostic capture is not an optimization command. Zero-sample sessions fail closed (",r.jsx("code",{className:"font-mono",children:"RECOVERY_REQUIRED"}),") and never fabricate passing device acceptance."]})]})},je=({label:o,value:c,note:m,simulated:d=!1})=>{const j=typeof c=="boolean";return r.jsxs("div",{className:"flex items-center justify-between py-2.5 last:border-b-0 text-xs",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"pr-3",children:[r.jsxs("span",{children:[o,d?" (sim)":""]}),m&&r.jsxs("span",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","· ",m]})]}),j?r.jsx("div",{className:"inline-flex items-center gap-1.5 font-bold font-mono tabular-nums shrink-0",style:{color:c?"var(--md-sys-color-success)":"var(--md-sys-color-on-surface-variant)"},children:c?r.jsxs(r.Fragment,{children:[r.jsx(au,{className:"w-3.5 h-3.5 shrink-0"}),r.jsx("span",{children:"Yes"})]}):r.jsxs(r.Fragment,{children:[r.jsx(sp,{className:"w-3.5 h-3.5 shrink-0 opacity-60"}),r.jsx("span",{children:"No"})]})}):r.jsx("span",{className:"font-bold font-mono tabular-nums shrink-0",children:c})]})},d0=({vulkanTelemetry:o,vulkanConfig:c,nativeMode:m,onSaveVulkanConfig:d,isSaving:j,saveMessage:O})=>{const N=wl(c,o),L=(R,x)=>{if(R)return x?"HW Native":void 0},B=async R=>{const g={...c,mode:R==="disabled"?"disabled":"safe_hardware"};await d(g)};return r.jsxs("div",{className:"space-y-6",children:[r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col lg:flex-row lg:items-start lg:justify-between gap-4",children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("span",{className:"md3-badge md3-badge-primary",children:m?"LIVE DEVICE ICD PROBE":"HARDWARE PASS-THROUGH MODEL (SIM)"}),r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:N.deviceName})]}),r.jsxs("h2",{className:"text-xl sm:text-2xl font-bold tracking-tight",children:["Vulkan Capabilities & Truthful Hardware Pass-Through",m?"":" (sim)"]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Dynamically probes the active device's physical GPU ICD (Qualcomm Adreno KGSL, ARM Mali / Immortalis, Samsung Xclipse, PowerVR) and reports genuine hardware capabilities without fabricating unsupported extensions, feature flags, or API versions. Opt-in diagnostic layer"," ",r.jsx("code",{className:"font-mono text-xs",children:"libVkLayer_coreflow.so"})," requires"," ",r.jsx("code",{className:"font-mono text-xs",children:"ENABLE_COREFLOW_VULKAN_LAYER=1"}),"."]})]}),r.jsxs("div",{className:"md3-card-filled px-4 py-3 text-left lg:text-right font-mono tabular-nums text-xs shrink-0 space-y-1",children:[r.jsxs("div",{children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Hardware API: "}),r.jsxs("span",{className:"font-bold",children:["Vulkan ",N.apiVersion]})]}),r.jsxs("div",{children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Device Extensions: "}),r.jsx("span",{className:"font-bold",children:N.deviceExtensions}),r.jsxs("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","· Instance: ",N.instanceExtensions]})]})]})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsx("div",{className:"text-xs font-bold",children:"Select Vulkan Layer Mode"}),r.jsxs("div",{className:"grid grid-cols-1 md:grid-cols-2 gap-3",children:[r.jsxs("button",{type:"button",id:"btn-vulkan-mode-disabled",disabled:j,"aria-pressed":c.mode==="disabled",onClick:()=>void B("disabled"),className:"text-left p-4 rounded-2xl transition-all",style:{backgroundColor:c.mode==="disabled"?"var(--md-sys-color-primary-container)":"var(--md-sys-color-surface-container)",color:c.mode==="disabled"?"var(--md-sys-color-on-primary-container)":"var(--md-sys-color-on-surface)",border:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 mb-1.5",children:[r.jsx("span",{className:"text-xs font-bold",children:"Default · Implicit Layer Disabled (Recommended)"}),r.jsx(Aa,{className:"w-4 h-4 shrink-0"})]}),r.jsxs("p",{className:"text-[11px] leading-relaxed opacity-85",children:["Keeps ",r.jsx("code",{className:"font-mono",children:"VK_LAYER_COREFLOW_universal_bridge"})," inactive for normal applications while still reading native GPU devfreq and physical ICD capabilities."]})]}),r.jsxs("button",{type:"button",id:"btn-vulkan-mode-passthrough",disabled:j,"aria-pressed":c.mode!=="disabled",onClick:()=>void B("safe_hardware"),className:"text-left p-4 rounded-2xl transition-all",style:{backgroundColor:c.mode!=="disabled"?"var(--md-sys-color-primary-container)":"var(--md-sys-color-surface-container)",color:c.mode!=="disabled"?"var(--md-sys-color-on-primary-container)":"var(--md-sys-color-on-surface)",border:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 mb-1.5",children:[r.jsx("span",{className:"text-xs font-bold",children:"Truthful Hardware Pass-Through"}),r.jsx(yg,{className:"w-4 h-4 shrink-0"})]}),r.jsxs("p",{className:"text-[11px] leading-relaxed opacity-85",children:["Uses per-handle 64-bit aligned Vulkan dispatch and forwards 100% authentic ICD capabilities without spoofing API versions, injecting extensions, or stripping ",r.jsx("code",{className:"font-mono",children:"vkCreateDevice"})," requests."]})]})]})]}),r.jsxs("div",{className:"flex flex-col gap-3 pt-4 sm:flex-row sm:items-center sm:justify-between",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"max-w-2xl text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("p",{className:"font-bold",style:{color:"var(--md-sys-color-on-surface)"},children:"Manual Vulkan Diagnostic Layer"}),r.jsx("p",{className:"mt-0.5",children:"This opt-in controls only the Vulkan diagnostic / pass-through layer. All CPU, memory, storage, and thermal policies remain daemon-owned."})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2.5",children:[j?r.jsx("span",{className:"text-xs font-semibold",children:"Applying Vulkan mode…"}):O&&r.jsx("span",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:O}),r.jsx("span",{className:c.mode==="disabled"?"md3-badge md3-badge-neutral":"md3-badge md3-badge-primary",children:c.mode==="disabled"?"Layer Disabled":"Layer Enabled"})]})]})]}),r.jsxs("div",{className:"grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6",children:[r.jsxs("div",{className:"md3-card-elevated p-6 space-y-4",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Capabilities & Identity"}),r.jsx("span",{className:"md3-badge md3-badge-primary font-mono",children:N.vendor})]}),r.jsxs("div",{children:[r.jsx(je,{simulated:!m,label:"Ray Tracing",value:N.rayTracing,note:L(N.rayTracing,N.hwRayTracing)}),r.jsx(je,{simulated:!m,label:"Mesh Shaders",value:N.meshShaders,note:L(N.meshShaders,N.hwMeshShaders)}),r.jsx(je,{simulated:!m,label:"Variable Rate Shading",value:N.variableRateShading,note:L(N.variableRateShading,N.hwVariableRateShading)}),r.jsx(je,{simulated:!m,label:"Cooperative Matrix",value:N.cooperativeMatrix,note:L(N.cooperativeMatrix,N.hwCooperativeMatrix)}),r.jsx(je,{simulated:!m,label:"RT Pipeline",value:N.rtPipeline,note:L(N.rtPipeline,N.hwRtPipeline)}),r.jsx(je,{simulated:!m,label:"Ray Query",value:N.rayQuery,note:L(N.rayQuery,N.hwRayQuery)}),r.jsx(je,{simulated:!m,label:"Acceleration Structure",value:N.accelerationStructure,note:L(N.accelerationStructure,N.hwAccelerationStructure)}),r.jsx(je,{simulated:!m,label:"Portability Subset",value:N.portabilitySubset}),r.jsx(je,{simulated:!m,label:"Device Extensions",value:N.deviceExtensions,note:"HW Native ICD"}),r.jsx(je,{simulated:!m,label:"Instance Extensions",value:N.instanceExtensions})]}),r.jsxs("div",{className:"pt-2 space-y-1.5 text-[11px] font-mono",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("div",{className:"truncate",title:N.deviceUuid,children:["Device UUID: ",N.deviceUuid]}),r.jsxs("div",{className:"truncate",title:N.driverUuid,children:["Driver UUID: ",N.driverUuid]}),r.jsxs("div",{className:"truncate",title:N.pipelineCacheUuid,children:["Cache UUID: ",N.pipelineCacheUuid]})]})]}),r.jsxs("div",{className:"md3-card-elevated p-6 space-y-4",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Core & Vulkan 1.1–1.3"}),r.jsx("span",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Spec Matrix"})]}),r.jsxs("div",{children:[r.jsx(je,{simulated:!m,label:"Robust Buffer Access",value:N.robustBufferAccess}),r.jsx(je,{simulated:!m,label:"Sampler Anisotropy",value:N.samplerAnisotropy}),r.jsx(je,{simulated:!m,label:"16-Bit Integers",value:N.shaderInt16}),r.jsx(je,{simulated:!m,label:"64-Bit Floats (FP64)",value:N.shaderFloat64,note:L(N.shaderFloat64,N.hwShaderFloat64)}),r.jsx(je,{simulated:!m,label:"Protected Memory",value:N.protectedMemory,note:"Vulkan 1.1"}),r.jsx(je,{simulated:!m,label:"Timeline Semaphore",value:N.timelineSemaphore,note:"Vulkan 1.2"}),r.jsx(je,{simulated:!m,label:"Descriptor Indexing",value:N.descriptorIndexing,note:"Vulkan 1.2"}),r.jsx(je,{simulated:!m,label:"Buffer Device Address",value:N.bufferDeviceAddress,note:"Vulkan 1.2"}),r.jsx(je,{simulated:!m,label:"Dynamic Rendering",value:N.dynamicRendering,note:"Vulkan 1.3"}),r.jsx(je,{simulated:!m,label:"Synchronization2",value:N.synchronization2,note:"Vulkan 1.3"})]})]}),r.jsxs("div",{className:"md3-card-elevated p-6 space-y-4",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Vulkan 1.4 & GPU Surface"}),r.jsxs("span",{className:"md3-badge md3-badge-secondary font-mono",children:["API ",N.apiVersion]})]}),r.jsxs("div",{children:[r.jsx(je,{simulated:!m,label:"8-Bit Indices (indexTypeUint8)",value:N.indexTypeUint8,note:L(N.indexTypeUint8,N.hwIndexTypeUint8)}),r.jsx(je,{simulated:!m,label:"Line Rasterization",value:N.lineRasterization,note:L(N.lineRasterization,N.hwLineRasterization)}),r.jsx(je,{simulated:!m,label:"Push Descriptors",value:N.pushDescriptors,note:L(N.pushDescriptors,N.hwPushDescriptors)}),r.jsx(je,{simulated:!m,label:"Global Priority Query",value:N.globalPriorityQuery,note:L(N.globalPriorityQuery,N.hwGlobalPriorityQuery)}),r.jsx(je,{simulated:!m,label:"Shader Float Controls 2",value:N.shaderFloatControls2,note:L(N.shaderFloatControls2,N.hwShaderFloatControls2)}),r.jsx(je,{simulated:!m,label:"Max Push Descriptors",value:N.maxPushDescriptors}),r.jsx(je,{simulated:!m,label:"Maintenance 5 / 6 / 7",value:`${N.maintenance5?"M5":"—"} · ${N.maintenance6?"M6":"—"} · ${N.maintenance7?"M7":"—"}`}),r.jsx(je,{simulated:!m,label:"Descriptor Buffer",value:N.descriptorBuffer,note:L(N.descriptorBuffer,N.hwDescriptorBuffer)}),r.jsx(je,{simulated:!m,label:"Graphics Pipeline Library",value:N.graphicsPipelineLibrary,note:L(N.graphicsPipelineLibrary,N.hwGraphicsPipelineLibrary)})]}),r.jsxs("div",{className:"pt-3 space-y-1.5",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"text-xs font-bold",children:["Multi-Vendor GPU Kernel Surface (",N.gpuKernelLabel,")"]}),r.jsx(je,{simulated:!m,label:"Active Governor",value:N.gpuGovernor}),r.jsx(je,{simulated:!m,label:"GPU Clock (Cur / Max)",value:`${N.gpuCurrentFreqMhz} / ${N.gpuMaxFreqMhz} MHz`}),r.jsx(je,{simulated:!m,label:"Adrenoboost / Pacing",value:`Level ${N.adrenoboostLevel}`})]})]})]})]})},_0=o=>o==="validated"?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",m0=({evidence:o,nativeConnected:c,simulationMode:m=!1})=>r.jsxs("section",{"aria-label":"Native semantic evidence",className:"md3-card-elevated p-6 sm:p-7 space-y-5",children:[r.jsxs("div",{className:"flex flex-wrap items-start justify-between gap-3",children:[r.jsxs("div",{className:"flex items-center gap-3.5",children:[r.jsx("div",{className:"w-11 h-11 rounded-2xl flex items-center justify-center shrink-0",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(sg,{className:"w-5 h-5"})}),r.jsxs("div",{children:[r.jsx("h3",{className:"text-base font-bold tracking-tight",children:"Native Semantic Evidence"}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Daemon-owned CapabilityResolver & 3-relation DomainEvidenceAggregator · Schema v1"})]})]}),c?r.jsxs("span",{className:"md3-badge md3-badge-success",children:[r.jsx(iu,{className:"w-3.5 h-3.5"})," Connected"]}):r.jsxs("span",{className:"md3-badge md3-badge-warning",children:[r.jsx(Mh,{className:"w-3.5 h-3.5"}),m?"Simulation Mode":"Awaiting Official Data"]})]}),m?r.jsxs("div",{role:"note",className:"rounded-2xl px-4 py-3.5 text-xs leading-relaxed",style:{backgroundColor:"var(--md-sys-color-warning-container)",color:"var(--md-sys-color-on-warning-container)"},children:[r.jsx("strong",{children:"Simulation Mode."})," Browser preview metrics are synthetic and labelled ",r.jsx("code",{className:"font-mono",children:"(sim)"}),". No kernel semantic binding or corroboration evidence is fabricated in the browser."]}):(o==null?void 0:o.status)==="unavailable"?r.jsxs("div",{className:"md3-card-filled px-4 py-3.5 flex gap-3 text-xs leading-relaxed",children:[r.jsx(Si,{className:"w-4 h-4 shrink-0 mt-0.5",style:{color:"var(--md-sys-color-warning)"}}),r.jsxs("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:["The daemon returned schema v1 with status ",r.jsx("code",{className:"font-mono",children:"unavailable"})," because the discovery adapter has not produced semantic evidence on this node set. No binding state or functional domain is fabricated."]})]}):c?o?r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs",children:[r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Generation"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold tabular-nums",children:o.generation??"N/A"})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Observed · monotonic ns"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold tabular-nums break-all",children:o.observedAtMonotonicNs??"N/A"})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Age"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold tabular-nums",children:o.ageMs===null?"N/A":`${o.ageMs.toFixed(3)} ms`})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Freshness"}),r.jsxs("div",{className:"mt-1 flex items-center gap-1.5 text-sm font-bold",children:[r.jsx(Lh,{className:"w-3.5 h-3.5"}),r.jsx("span",{style:{color:o.freshness==="fresh"?"var(--md-sys-color-success)":"var(--md-sys-color-warning)"},children:o.freshness})]})]})]}),r.jsx("div",{className:"space-y-2.5",children:o.bindings.map((d,j)=>r.jsxs("article",{className:"md3-card-outlined p-4 space-y-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("div",{className:"flex items-center gap-2 text-xs font-bold",children:[r.jsx(ru,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Semantic Binding ",j+1]})]}),r.jsx("span",{className:_0(d.bindingState),children:d.bindingState.toUpperCase()})]}),r.jsxs("div",{className:"grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Domain"}),r.jsx("div",{className:"mt-1 font-bold uppercase",children:d.domain})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Assessment"}),r.jsx("div",{className:"mt-1 font-bold",children:d.domainAssessment.state})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Confidence"}),r.jsxs("div",{className:"mt-1 font-mono font-bold tabular-nums",children:[(d.domainAssessment.confidence*100).toFixed(1),"%"]})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Corroborating relations"}),r.jsxs("div",{className:"mt-1 font-mono font-bold tabular-nums",children:[d.domainAssessment.corroboratingRelations,"/3"]})]})]}),r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 pt-2.5 text-[11px]",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["Binding freshness:"," ",r.jsx("strong",{style:{color:d.freshness==="fresh"?"var(--md-sys-color-success)":"var(--md-sys-color-warning)"},children:d.freshness}),d.ageMs===null?"":` · ${d.ageMs.toFixed(3)} ms`]}),r.jsxs("span",{className:"inline-flex items-center gap-1",children:[r.jsx(On,{className:"w-3.5 h-3.5"}),"Semantic validation is not mutation authority"]})]})]},`${d.generation}-${d.observedAtMonotonicNs}-${j}`))})]}):r.jsx("div",{className:"md3-card-filled px-4 py-3.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No official semantic telemetry is available."}):r.jsx("div",{className:"md3-card-filled px-4 py-3.5 text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Native bridge detected, but an official schema v1 daemon status payload has not yet been received. Semantic bindings remain unavailable until the daemon responds."})]}),eu=o=>typeof o=="object"&&o!==null&&!Array.isArray(o),fp=o=>typeof o=="number"&&Number.isFinite(o),pp=o=>o===null||fp(o),Wf=o=>o==="unresolved"||o==="cpu"||o==="gpu"||o==="vm",is=o=>typeof o=="string"&&/^[0-9]+$/.test(o),vp=o=>o==="fresh"||o==="stale",f0=o=>o==="unresolved"||o==="candidate"||o==="corroborated"||o==="verified",p0=o=>{if(!eu(o)||!eu(o.domainAssessment))return!1;const c=o.domainAssessment;return(o.bindingState==="candidate"||o.bindingState==="validated")&&Wf(o.domain)&&f0(c.state)&&Wf(c.domain)&&fp(c.confidence)&&c.confidence>=0&&c.confidence<=1&&Number.isInteger(c.corroboratingRelations)&&c.corroboratingRelations>=0&&c.corroboratingRelations<=3&&is(o.generation)&&is(o.observedAtMonotonicNs)&&pp(o.ageMs)&&(o.ageMs===null||o.ageMs>=0)&&vp(o.freshness)},v0=o=>!eu(o)||o.schemaVersion!==1||o.status!=="available"&&o.status!=="unavailable"||!vp(o.freshness)||!Array.isArray(o.bindings)||!o.bindings.every(p0)?!1:o.status==="unavailable"?o.generation===null&&o.observedAtMonotonicNs===null&&o.ageMs===null&&o.freshness==="stale"&&o.bindings.length===0:is(o.generation)&&is(o.observedAtMonotonicNs)&&pp(o.ageMs)&&(o.ageMs===null||o.ageMs>=0)&&o.bindings.length>0,es='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"',An="/data/adb/coreflow/coreflow.conf",y0="/data/adb/coreflow/logs/coreflowd.log",tu=o=>{const c=o.trim(),m=c.split(`
`).map(N=>N.trim()).filter(Boolean);for(let N=m.length-1;N>=0;N--){const L=m[N];if(L.startsWith("{")&&L.endsWith("}"))try{return JSON.parse(L)}catch{}}const d=c.indexOf('{"running":'),j=d>=0?d:c.indexOf("{"),O=c.lastIndexOf("}");if(j>=0&&O>j){const N=c.slice(j,O+1).replace(/\r?\n/g,"");return JSON.parse(N)}return JSON.parse(m[m.length-1]??"{}")},wi=o=>typeof o=="object"&&o!==null&&!Array.isArray(o),Ft=o=>typeof o=="number"&&Number.isFinite(o),h0=["osMajor","baselineNodesSnapshotted","thermalPressure","thermalSeverity","aiPredictedPressure10s","aiPredictedVelocity","aiRecommendedScale","aiConfidence","aiSamples","tunerUtilityDelta","tunerTrials","tunerImproved","tunerRolledBack"],g0=["cpuPressurePct","memoryPressurePct","ioPressurePct","socCompositeCelsius","cpuAverageCelsius","cpuPeakCelsius","gpuTempCelsius","batteryTempCelsius","skinTempCelsius","thermalHeadroom"],b0=["tier1Eligible","inspectionMode","mutationRuntimeStarted","cpuPressureValid","memoryPressureValid","ioPressureValid","thermalPolicyAvailable","thermalGuardActive","thermalDataDegraded","thermalFallbackActive","thermalHeadroomValid","aiOnnxActive"],x0=["platformVendor","osFamily","executionTier","policyFamily","identityConfidence","thermalSource","thermalSensor","aiBackend","tunerRuntimeStage","tunerStage","tunerWorkload","tunerBottleneck","tunerObjective","tunerIntent","tunerCandidateId","tunerCandidateLabel","tunerCapabilityFingerprint","tunerDecisionPlanRationale","tunerRationale","tunerVerdict"],xl=(o,c)=>c.every(m=>typeof o[m]=="string"),ts=(o,c)=>c.every(m=>Ft(o[m])),ep=(o,c)=>c.every(m=>typeof o[m]=="boolean"),w0={thermalSensors:o=>wi(o)&&xl(o,["id","type"])&&Ft(o.category)&&(o.celsius===null||Ft(o.celsius))&&ep(o,["valid","policyAvailable"])&&(o.velocityCelsiusPerSec===void 0||o.velocityCelsiusPerSec===null||Ft(o.velocityCelsiusPerSec)),blockQueues:o=>wi(o)&&xl(o,["device","activeScheduler","baselineScheduler"])&&Array.isArray(o.availableSchedulers)&&o.availableSchedulers.every(c=>typeof c=="string")&&ts(o,["readAheadKb","nrRequests","rqAffinity","readExpireMs","writesStarved","fifoBatch"]),tunerCandidates:o=>wi(o)&&xl(o,["id","label","domain"])&&ts(o,["trials","successes","rollbacks","meanDelta","ucbScore","cooldownTicks"]),tunerEvaluatedCandidates:o=>wi(o)&&xl(o,["id","domain","intent","rejectionReason"])&&ts(o,["utilityScore","empiricalScore","confidence","riskPenalty","thermalPenalty"])&&ep(o,["eligible","selected"]),tunerActiveInterventions:o=>wi(o)&&xl(o,["id","domain","intent","stage","rationale"])&&ts(o,["remainingTicks"])},N0=o=>{const c=(m,d)=>{o[m]!==void 0&&!d&&delete o[m]};for(const m of h0)c(m,Ft(o[m]));for(const m of g0)c(m,o[m]===null||Ft(o[m]));for(const m of b0)c(m,typeof o[m]=="boolean");for(const m of x0)c(m,typeof o[m]=="string");for(const[m,d]of Object.entries(w0)){const j=o[m];c(m,Array.isArray(j)&&j.every(d))}c("vulkan",wi(o.vulkan))},S0=o=>{const c=tu(o);if(typeof c!="object"||c===null)throw new Error("Invalid native status");const m=c;if(typeof m.running!="boolean"||!Ft(m.pid)||!Ft(m.uclampMin)||!Ft(m.uclampMax)||typeof m.uclampSupported!="boolean"||m.uclampPolicyMin!==void 0&&!Ft(m.uclampPolicyMin)||m.uclampPolicyMax!==void 0&&!Ft(m.uclampPolicyMax)||m.uclampTargetPath!==void 0&&typeof m.uclampTargetPath!="string"||m.uclampEncoding!==void 0&&typeof m.uclampEncoding!="string"||!Ft(m.socTempCelsius)&&m.socTempCelsius!==null||typeof m.recoveryPending!="boolean")throw new Error("Unexpected daemon status schema");if(m.semanticEvidence!==void 0&&!v0(m.semanticEvidence))throw new Error("Unexpected semanticEvidence schema; expected native schemaVersion 1");if(m.directAdaptive!==void 0&&!Vg(m.directAdaptive))throw new Error("Unexpected directAdaptive status schema");return N0(c),m},xi=async o=>{const c=await ji(o);if(c.errno!==0)throw new Error(c.stderr||c.stdout||"Native command failed");return c.stdout},j0=()=>{var ge;const[o]=Q.useState(()=>ls()),[c,m]=Q.useState(()=>Dg()),[d,j]=Q.useState(null),[O,N]=Q.useState(null),[L,B]=Q.useState(null),[R,x]=Q.useState(!1),[g,q]=Q.useState(!1),$=o&&g,[M,X]=Q.useState(()=>{const A=Lg();return o?{...A,engineActive:!1,uclampMin:0,uclampMax:0,uclampSupported:!1,socTempCelsius:-1,cpuPressurePct:0,memoryPressurePct:0,ioPressurePct:0,cpuPressureValid:!1,memoryPressureValid:!1,ioPressureValid:!1,baselineNodesSnapshotted:0,baselineStatus:"restored",transactionStage:"PLANNED",modeLabel:"Awaiting native status",modeDescription:"Waiting for coreflowd runtime status",vulkan:wl($c)}:A}),[I,J]=Q.useState(Sl.thermalTripCelsius),[ae,F]=Q.useState("dashboard"),[re,xe]=Q.useState(()=>({...$c})),[C,z]=Q.useState(null),[P,ye]=Q.useState(()=>o?[]:Hg()),[Re,Me]=Q.useState(!1),[Ne,$e]=Q.useState({socModel:"Unavailable",kernelVersion:"Unavailable",onlineCpus:"Unavailable",gpuType:"Unavailable"});Q.useEffect(()=>{o&&xi(`${es}; "$BIN" --capabilities`).then(A=>{const U=tu(A);U&&["socModel","kernelVersion","onlineCpus","gpuType"].every(E=>typeof U[E]=="string")&&$e(U)}).catch(()=>{})},[o]);const[it,Le]=Q.useState(!1),H=up(c.theme);Q.useEffect(()=>{dp(c)},[c]);const ne=()=>{const U={...c,theme:H?"light":"dark"};m(U),Kf(U)},Z=A=>{const U={...c,accent:A};m(U),Kf(U)},Ee=Q.useMemo(()=>mp(M,I),[M,I]),he=Q.useRef(M);he.current=M;const gt=Q.useRef(0),bt=Q.useCallback((A,U,E)=>{gt.current+=1;const we={id:`evt-${Date.now()}-${gt.current}`,timestamp:new Date().toTimeString().slice(0,8),type:A,tag:U,message:E};ye(Ze=>[we,...Ze].slice(0,50))},[]),xt=Q.useCallback(async()=>{var A,U;try{const E=S0(await xi(`${es}; "$BIN" --status`));N(E.semanticEvidence??null),q(E.semanticEvidence!==void 0);const we=E.socCompositeCelsius??E.socTempCelsius??-1,Ze=((A=E.directAdaptive)==null?void 0:A.thermalGuard)??E.thermalGuardActive??(E.thermalFallbackActive?we>=I:(E.thermalSeverity??0)>=3),We=(U=E.directAdaptive)==null?void 0:U.mode,Te=We?{SCREEN_OFF:"Screen Off / Deep Idle",BALANCED_SWEET_SPOT:"Balanced Sweet-Spot",ACTIVE_LOAD:"Active Load / Gaming",THERMAL_GUARD:"Thermal Guard",DISCOVERY:"Capability Discovery"}[We]??We:void 0;return X(V=>{var Ye,lt,rt,wt,nn;return{...V,engineActive:E.running&&(((Ye=E.directAdaptive)==null?void 0:Ye.active)??!0),directAdaptive:E.directAdaptive,platformVendor:E.platformVendor??V.platformVendor,osFamily:E.osFamily??V.osFamily,osMajor:E.osMajor??V.osMajor,executionTier:E.executionTier??V.executionTier,policyFamily:E.policyFamily??V.policyFamily,identityConfidence:E.identityConfidence??V.identityConfidence,tier1Eligible:E.tier1Eligible??V.tier1Eligible,inspectionMode:E.inspectionMode??V.inspectionMode,mutationRuntimeStarted:E.mutationRuntimeStarted??V.mutationRuntimeStarted,uclampMin:E.uclampMin,uclampMax:E.uclampMax,uclampSupported:E.uclampSupported,uclampPolicyMin:E.uclampPolicyMin??V.uclampPolicyMin,uclampPolicyMax:E.uclampPolicyMax??V.uclampPolicyMax,uclampTargetPath:E.uclampTargetPath??V.uclampTargetPath,uclampEncoding:E.uclampEncoding??V.uclampEncoding,cpuPressurePct:E.cpuPressurePct??0,memoryPressurePct:E.memoryPressurePct??0,ioPressurePct:E.ioPressurePct??0,cpuPressureValid:E.cpuPressureValid??E.cpuPressurePct!=null,memoryPressureValid:E.memoryPressureValid??E.memoryPressurePct!=null,ioPressureValid:E.ioPressureValid??E.ioPressurePct!=null,baselineNodesSnapshotted:E.baselineNodesSnapshotted??V.baselineNodesSnapshotted,socTempCelsius:we,socCompositeCelsius:E.socCompositeCelsius??we,cpuAverageCelsius:E.cpuAverageCelsius??-1,cpuPeakCelsius:E.cpuPeakCelsius??-1,gpuTempCelsius:E.gpuTempCelsius??-1,batteryTempCelsius:E.batteryTempCelsius??-1,skinTempCelsius:E.skinTempCelsius??-1,thermalGuardActive:Ze,thermalDataDegraded:E.thermalDataDegraded??!1,thermalFallbackActive:E.thermalFallbackActive??!1,thermalHeadroomValid:E.thermalHeadroomValid??!1,thermalHeadroom:E.thermalHeadroom??null,thermalPressure:E.thermalPressure??V.thermalPressure,thermalSource:E.thermalSource??V.thermalSource,thermalSeverity:E.thermalSeverity??V.thermalSeverity,thermalSensors:E.thermalSensors??V.thermalSensors,aiBackend:E.aiBackend??V.aiBackend,aiOnnxActive:E.aiOnnxActive??V.aiOnnxActive,aiPredictedPressure10s:E.aiPredictedPressure10s??V.aiPredictedPressure10s,aiPredictedVelocity:E.aiPredictedVelocity??V.aiPredictedVelocity,aiRecommendedScale:E.aiRecommendedScale??V.aiRecommendedScale,aiConfidence:E.aiConfidence??V.aiConfidence,aiSamples:E.aiSamples??V.aiSamples,tunerRuntimeStage:E.tunerRuntimeStage??V.tunerRuntimeStage,tunerStage:E.tunerStage??V.tunerStage,tunerWorkload:E.tunerWorkload??V.tunerWorkload,tunerBottleneck:E.tunerBottleneck??V.tunerBottleneck,tunerObjective:E.tunerObjective??V.tunerObjective,tunerIntent:E.tunerIntent??V.tunerIntent,tunerCandidateId:E.tunerCandidateId??V.tunerCandidateId,tunerCandidateLabel:E.tunerCandidateLabel??V.tunerCandidateLabel,tunerCapabilityFingerprint:E.tunerCapabilityFingerprint??V.tunerCapabilityFingerprint,tunerDecisionPlanRationale:E.tunerDecisionPlanRationale??V.tunerDecisionPlanRationale,tunerRationale:E.tunerRationale??V.tunerRationale,tunerVerdict:E.tunerVerdict??V.tunerVerdict,tunerUtilityDelta:E.tunerUtilityDelta??V.tunerUtilityDelta,tunerTrials:E.tunerTrials??V.tunerTrials,tunerImproved:E.tunerImproved??V.tunerImproved,tunerRolledBack:E.tunerRolledBack??V.tunerRolledBack,blockQueues:E.blockQueues??V.blockQueues,tunerCandidates:E.tunerCandidates??V.tunerCandidates,tunerEvaluatedCandidates:E.tunerEvaluatedCandidates??V.tunerEvaluatedCandidates,tunerActiveInterventions:E.tunerActiveInterventions??V.tunerActiveInterventions,vulkan:E.vulkan??V.vulkan,baselineStatus:E.recoveryPending||((lt=E.directAdaptive)==null?void 0:lt.active)===!0&&E.directAdaptive.baselineNodes>0?"protected":"restored",transactionStage:E.running?E.recoveryPending?"RETAINED":"VERIFIED":"PLANNED",modeLabel:E.running?Te??(Ze?"Thermal Guard":"Direct Adaptive runtime active"):"Daemon Stopped",modeDescription:E.running?(rt=E.directAdaptive)!=null&&rt.journalFault?"Journal fault latched; all further Direct Adaptive mutations are halted until baseline recovery.":(wt=E.directAdaptive)!=null&&wt.ownershipLosses?"OEM contention isolated at affected nodes; independent capabilities continue.":(nn=E.directAdaptive)!=null&&nn.active?"Direct Baseline with capability-derived Sweet-Spot policy and verified readback.":"Daemon is running; Direct Adaptive discovery or baseline preparation is unavailable.":"OEM baseline restored",timestamp:Date.now()}}),j(null),E}catch(E){throw q(!1),N(null),X(we=>({...we,directAdaptive:void 0})),E}},[I]);Q.useEffect(()=>{if(!o)return;let A=!1;const U=async()=>{const Te=await ji(`cat ${An} 2>/dev/null`);if(A||Te.errno!==0)return;const V=Ug(Te.stdout,"thermal_trip_celsius");V!==null&&J(Math.max(30,Math.min(80,V)));const Ye=Wc(Te.stdout,"vulkan_layer_mode"),lt=Ye==="safe_hardware"||Ye==="full_capability_override"?"safe_hardware":"disabled",rt=wt=>{const nn=Wc(Te.stdout,wt);return nn==="true"?!0:nn==="false"?!1:null};xe(wt=>({...wt,mode:lt,enableSkiavkThreaded:rt("enable_skiavk_threaded")??wt.enableSkiavkThreaded,enableAdaptiveVrs:rt("enable_adaptive_vrs")??wt.enableAdaptiveVrs,enablePipelineCachePrewarm:rt("enable_pipeline_cache_prewarm")??wt.enablePipelineCachePrewarm}))};let E=!1,we="";const Ze=async()=>{if(!(E||A)){E=!0;try{await xt();const Te=await ji(`/system/bin/tail -n 12 ${y0} 2>/dev/null`);if(!A&&Te.errno===0){const V=Te.stdout.trim();if(V&&V!==we){we=V;const Ye=V.split(/\r?\n/).map(lt=>lt.trim()).filter(Boolean).slice(-12).reverse();ye(Ye.map((lt,rt)=>({id:`native-${rt}-${lt}`,timestamp:"--:--:--",type:lt.includes("[Stability]")&&lt.includes("thermal")?"THERMAL":"INFO",tag:"Daemon",message:lt.slice(0,384)})))}}}catch(Te){A||j(String(Te))}finally{E=!1}}};U(),Ze();const We=window.setInterval(()=>void Ze(),1e4);return()=>{A=!0,window.clearInterval(We)}},[o,xt]);const p=Q.useCallback(A=>{const{next:U,event:E}=Gg(he.current,Sl,A);he.current=U,X(U),E&&ye(we=>[E,...we].slice(0,50))},[]);Q.useEffect(()=>{if(o)return;const A=window.setInterval(()=>{p()},2500);return()=>window.clearInterval(A)},[o,p]);const D=async A=>{const U=await ji(`cat "${An}" 2>/dev/null || cat /data/adb/modules/cfae_framework/system/etc/coreflow/default.conf 2>/dev/null || cat /system/etc/coreflow/default.conf 2>/dev/null || true`),E=Bg(U.stdout??"",{vulkan_layer_mode:A}),we=E.endsWith(`
`)?E:E+`
`;let Ze="COREFLOW_CONFIG";for(let V=1;we.split(`
`).some(Ye=>Ye.trim()===Ze);V++)Ze=`COREFLOW_CONFIG_${V}`;const We='umask 077 && [ ! -L "'+An+'" ] && rm -f "'+An+`.tmp" && cat <<'${Ze}' > `+An+`.tmp
`+we+Ze+`
chown 0:0 `+An+".tmp 2>/dev/null; chmod 600 "+An+".tmp && mv -f "+An+".tmp "+An+" && echo COREFLOW_VULKAN_WRITTEN";if(!(await xi(We)).includes("COREFLOW_VULKAN_WRITTEN"))throw new Error("Vulkan layer mode write not acknowledged")},W=async A=>{Le(!0),z(null);try{const U=A.mode==="disabled"?"disabled":"safe_hardware",E={...A,mode:U};if(!o){xe(E),X(we=>({...we,vulkan:wl(E,we.vulkan)})),z(U==="safe_hardware"?"Truthful Hardware Pass-Through active in preview":"OEM Passthrough (Disabled) active in preview"),bt("BOOST","VulkanLayer",`Layer mode=${U}; rendering optimizers remain unimplemented.`);return}await D(U),xe(E),X(we=>({...we,vulkan:wl(E,we.vulkan)})),await xi("killall -HUP coreflowd 2>/dev/null || true"),z("Mode saved · daemon reload requested"),xt().catch(()=>{}),bt("BOOST","VulkanLayer",`Vulkan layer mode changed to ${U}; no system tuning policy was changed.`)}catch(U){z("Save failed"),j(String(U))}finally{Le(!1)}},ee=Q.useCallback(async()=>{try{const A=await xi(`${es}; "$BIN" --journal`),U=tu(A);B(JSON.stringify(U,null,2)),j(null)}catch(A){j("Unable to read the mutation journal: "+String(A))}},[]),pe=Q.useCallback(async()=>{if(window.confirm("Restore every CoreFlow-owned node to its captured OEM baseline and stop coreflowd?")){x(!0);try{await xi(`${es}; "$BIN" --rollback-baseline`),B(null),await xt(),bt("BASELINE","Baseline","OEM baseline rollback requested from the Direct Adaptive audit panel.")}catch(A){j("OEM baseline restore failed: "+String(A))}finally{x(!1)}}},[bt,xt]),ve=[{id:"dashboard",label:"Overview",shortLabel:"Overview",Icon:ra},{id:"thermal",label:"Thermal Matrix",shortLabel:"Thermal",Icon:ns},{id:"vulkan",label:"Vulkan & GPU",shortLabel:"Vulkan",Icon:su},{id:"hardware",label:"Hardware & CDDP",shortLabel:"Hardware",Icon:Nl},{id:"validation",label:"DVS Validation",shortLabel:"Validation",Icon:Bh},{id:"console",label:"Audit & RPC",shortLabel:"Audit",Icon:uu}];return r.jsxs("div",{className:"min-h-screen relative overflow-x-hidden flex flex-col font-sans antialiased",style:{background:"var(--md-sys-color-background)",color:"var(--md-sys-color-on-surface)"},children:[r.jsx("div",{role:"status",className:"px-4 py-2 text-center text-xs font-medium",style:{background:o&&$?"var(--md-sys-color-success-container)":"var(--md-sys-color-warning-container)",color:o&&$?"var(--md-sys-color-on-success-container)":"var(--md-sys-color-on-warning-container)",borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:o?$?"Connected · Native bridge and official daemon status with semanticEvidence schema v1 received.":"Native bridge detected · Awaiting an official daemon status payload; semantic telemetry is unavailable.":"Simulation Mode · Browser-only preview; synthetic values are labelled (sim) and are not kernel audit evidence."}),d&&r.jsx("div",{role:"alert",className:"px-4 py-2 text-xs font-medium text-center",style:{background:"var(--md-sys-color-error-container)",color:"var(--md-sys-color-on-error-container)"},children:d}),r.jsx(Zg,{isDark:H,accent:c.accent,onToggleTheme:ne,onSelectAccent:Z,modeLabel:M.modeLabel,nativeMode:o,nativeConnected:$,equilibriumScore:Ee.equilibriumScore,onOpenArchitectureStudio:()=>Me(!0)}),r.jsxs("div",{className:"flex-1 w-full max-w-[1440px] mx-auto px-3 sm:px-6 py-5 sm:py-7 flex flex-col lg:flex-row gap-6",children:[r.jsx("aside",{className:"hidden lg:block w-56 shrink-0",children:r.jsxs("nav",{"aria-label":"Primary sections",className:"md3-nav-rail sticky top-24 space-y-1.5",children:[r.jsx("div",{className:"px-3 py-2 text-[10px] font-bold uppercase tracking-[0.16em]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"System Domains"}),ve.map(({id:A,label:U,Icon:E})=>{const we=ae===A;return r.jsxs("button",{type:"button","aria-current":we?"page":void 0,onClick:()=>F(A),className:"w-full flex items-center gap-3 px-3.5 py-2.5 rounded-full text-xs font-medium transition-colors text-left "+(we?"font-semibold":""),style:{background:we?"var(--md-sys-color-secondary-container)":"transparent",color:we?"var(--md-sys-color-on-secondary-container)":"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(E,{className:"w-4 h-4 shrink-0"}),r.jsx("span",{className:"truncate",children:U})]},A)}),r.jsxs("div",{className:"mt-4 pt-3 px-3 space-y-1.5 text-[11px]",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("div",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"Execution Tier"}),r.jsx("div",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-primary)"},children:M.executionTier??"TIER_1_FULL_COORDINATED"}),r.jsx("div",{className:"text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:M.policyFamily??"android_linux_v4"})]})]})}),r.jsx("nav",{"aria-label":"Primary sections mobile",className:"lg:hidden md3-bottom-nav flex items-center gap-1.5 overflow-x-auto no-scrollbar max-w-full",children:ve.map(({id:A,shortLabel:U,Icon:E})=>{const we=ae===A;return r.jsxs("button",{type:"button","aria-current":we?"page":void 0,onClick:()=>F(A),className:"md3-chip flex items-center gap-1.5 whitespace-nowrap select-none "+(we?"md3-chip-active":""),children:[r.jsx(E,{className:"w-3.5 h-3.5 shrink-0"}),r.jsx("span",{children:U})]},A)})}),r.jsxs("main",{className:"flex-1 min-w-0 space-y-6",children:[ae==="dashboard"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("section",{className:"space-y-4","aria-label":"Autonomous runtime state",children:[r.jsxs("div",{className:"flex flex-col gap-2 sm:flex-row sm:items-end sm:justify-between",children:[r.jsxs("div",{children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"CFAE · Material Design 3 System Console"}),r.jsx("h1",{className:"mt-1 text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Autonomous Operating State"}),r.jsx("p",{className:"mt-1 max-w-2xl text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Capability discovery, evidence-led tuning, and verified actuator ownership. The console reflects native daemon telemetry while selection remains autonomous and safety-gated."})]}),r.jsx("div",{className:"flex flex-wrap items-center gap-2",children:r.jsxs("span",{className:"md3-badge "+(M.engineActive?"md3-badge-success":"md3-badge-neutral"),children:[r.jsx("span",{className:"h-1.5 w-1.5 rounded-full",style:{background:M.engineActive?"var(--md-sys-color-success)":"var(--md-sys-color-outline)"}}),M.engineActive?"Autonomous Engine Active":"Daemon Standby"]})})]}),r.jsx(Yg,{telemetry:M,nativeMode:o,thermalTripCelsius:I,onTriggerSimulation:o?void 0:p}),o&&r.jsx(Kg,{status:M.directAdaptive,nativeMode:!0,journalOutput:L,restoringBaseline:R,onViewJournal:ee,onRestoreBaseline:pe})]}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"md3-card-elevated flex items-start gap-3.5 p-4",children:[r.jsx("div",{className:"rounded-2xl p-2.5 shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(rp,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Thermal Headroom"}),r.jsx("div",{className:"mt-1 text-lg font-semibold tabular-nums",style:{color:"var(--md-sys-color-on-surface)"},children:M.thermalHeadroomValid&&M.thermalHeadroom!=null?`${Math.round(M.thermalHeadroom*100)}%`:"Unavailable"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:M.thermalGuardActive?"Thermal guard engaged":"OEM thermal policy respected"})]})]}),r.jsxs("div",{className:"md3-card-elevated flex items-start gap-3.5 p-4",children:[r.jsx("div",{className:"rounded-2xl p-2.5 shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(as,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Resource Pressure"}),r.jsx("div",{className:"mt-1 text-lg font-semibold tabular-nums",style:{color:"var(--md-sys-color-on-surface)"},children:M.cpuPressureValid||M.memoryPressureValid||M.ioPressureValid?`${Math.max(M.cpuPressureValid?M.cpuPressurePct:0,M.memoryPressureValid?M.memoryPressurePct:0,M.ioPressureValid?M.ioPressurePct:0).toFixed(1)}%`:"Unavailable"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Highest available PSI avg10"})]})]}),r.jsxs("div",{className:"md3-card-elevated flex items-start gap-3.5 p-4",children:[r.jsx("div",{className:"rounded-2xl p-2.5 shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(ou,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Mutation Ownership"}),r.jsx("div",{className:"mt-1 text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:M.baselineStatus==="protected"?"Journal Armed":"Guarded"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CFJ4 Baseline + Readback guard"})]})]})]}),r.jsx(m0,{evidence:O,nativeConnected:$,simulationMode:!o}),!o&&r.jsx(Jf,{nodesSnapshotted:M.baselineNodesSnapshotted,nativeMode:!1,baselineStatus:M.baselineStatus}),r.jsx(Wr,{logs:P,nativeMode:o})]}),ae==="thermal"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Protection Domain"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Thermal Intelligence & Multi-Zone Matrix"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Sensor evidence, velocity tracking, and OEM thermal constraints are surfaced for full operational transparency. Mitigation is selected autonomously."})]}),r.jsx(r0,{telemetry:M,nativeMode:o,thermalTripCelsius:I}),r.jsx(Wr,{logs:P.filter(A=>A.type==="THERMAL"||A.tag.toLowerCase().includes("thermal")),nativeMode:o})]}),ae==="vulkan"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"User Opt-In Subsystem"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Vulkan Diagnostics & Explicit Layer"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Vulkan layer activation (`VK_LAYER_COREFLOW_optimizer`) is the sole manual feature switch in CoreFlow. All kernel tuning domains remain strictly autonomous."})]}),r.jsx(d0,{vulkanTelemetry:M.vulkan,vulkanConfig:re,nativeMode:o,onSaveVulkanConfig:W,isSaving:it,saveMessage:C}),r.jsx(Wr,{logs:P.filter(A=>A.tag.toLowerCase().includes("vulkan")),nativeMode:o})]}),ae==="hardware"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Capability Discovery & CDDP"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Hardware & Platform Identity Profile"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Detected hardware is evidence, not automatic mutation authority. CoreFlow adapts only through validated operational contracts."})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx("div",{className:"w-9 h-9 rounded-2xl flex items-center justify-center",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(Aa,{className:"h-4 w-4"})}),r.jsxs("div",{children:[r.jsx("h2",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Device Identity"}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Probed via `coreflowd --capabilities`"})]})]}),r.jsx("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[["SoC Model",Ne.socModel],["GPU Subsystem",Ne.gpuType],["Kernel Release",Ne.kernelVersion],["Online CPUs",Ne.onlineCpus]].map(([A,U])=>r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:A}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:U})]},A))})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 flex-wrap",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx("div",{className:"w-9 h-9 rounded-2xl flex items-center justify-center",style:{background:"var(--md-sys-color-secondary-container)",color:"var(--md-sys-color-on-secondary-container)"},children:r.jsx(ru,{className:"h-4 w-4"})}),r.jsxs("div",{children:[r.jsx("h2",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Platform Context & Execution Tier"}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Cross-platform policy gating & capability fingerprint"})]})]}),r.jsx("span",{className:"md3-badge font-mono "+(M.tier1Eligible!==!1?"md3-badge-success":"md3-badge-warning"),children:M.tier1Eligible!==!1?"TIER-1 ELIGIBLE":"RESTRICTED TIER"})]}),r.jsx("div",{className:"grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs",children:[["Platform Vendor",M.platformVendor??"Generic / Android"],["OS Family",`${M.osFamily??"Android"}${M.osMajor?` ${M.osMajor}`:""}`],["Execution Tier",M.executionTier??"TIER_1_FULL_COORDINATED"],["Policy Family",M.policyFamily??"android_linux_v4"],["Identity Confidence",M.identityConfidence??"HIGH"],["Capability Fingerprint",M.tunerCapabilityFingerprint??"cf-cap-verified"]].map(([A,U])=>r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:A}),r.jsx("div",{className:"mt-1 font-mono font-semibold break-all",style:{color:"var(--md-sys-color-on-surface)"},children:U})]},A))})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx("div",{className:"w-9 h-9 rounded-2xl flex items-center justify-center",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(On,{className:"h-4 w-4"})}),r.jsxs("div",{children:[r.jsx("p",{className:"text-[10px] font-bold uppercase tracking-[0.16em]",style:{color:"var(--md-sys-color-primary)"},children:"Kernel Contract Model"}),r.jsx("h2",{className:"text-base font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"Discovery Does Not Grant Mutation Authority"})]})]}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Every discovered attribute receives an explicit contract classification. Unknown nodes stay visible for hardware understanding but fail closed for automated writes."}),r.jsx("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[{name:"RUNTIME_TUNABLE",badge:"md3-badge-success",detail:"Only class eligible for tuning after ABI, range, access and baseline verification."},{name:"OBSERVATION_ONLY",badge:"md3-badge-primary",detail:"Hardware limits, topology, alignment and telemetry contribute to the fingerprint."},{name:"TEST_ONLY",badge:"md3-badge-warning",detail:"Writable testing interface, excluded from daily performance optimization."},{name:"PROTECTED",badge:"md3-badge-error",detail:"Integrity-critical or system-sensitive attribute; automatic mutation prohibited."}].map(A=>r.jsxs("div",{className:"md3-card-outlined p-4",children:[r.jsx("span",{className:"md3-badge font-mono "+A.badge,children:A.name}),r.jsx("p",{className:"mt-2 text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:A.detail})]},A.name))})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(Nl,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Autonomous Hardware Preservation"})]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Battery, storage, and I/O protection are evaluated from live workload and device evidence. No manual thresholds or scheduler presets are exposed here; unsupported nodes remain observation-only until they earn mutation authority."}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Battery Temperature"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:M.batteryTempCelsius!=null&&M.batteryTempCelsius>=0?`${M.batteryTempCelsius.toFixed(1)} °C`:"N/A"})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Block Queues Discovered"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:((ge=M.blockQueues)==null?void 0:ge.length)??0})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Baseline Snapshots"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:M.baselineNodesSnapshotted})]})]})]}),r.jsx(Jf,{nodesSnapshotted:M.baselineNodesSnapshotted,nativeMode:o,baselineStatus:M.baselineStatus})]}),ae==="validation"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Evidence, Not Assumptions"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Device Validation Suite (CF-DVS)"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Inspect the live validation contract, 8 invariant pass criteria, and native subsystem readiness."})]}),r.jsx(u0,{nativeMode:o})]}),ae==="console"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Runtime Transparency"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Read-Only Audit & Diagnostic RPC"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Query read-only runtime evidence, CFJ4 journals, DVS validation payloads, and decision logs directly from `coreflowd`."})]}),r.jsx(s0,{nativeMode:o,onRefreshTelemetry:async()=>{await xt()}}),r.jsx(Wr,{logs:P,nativeMode:o})]})]})]}),r.jsx(l0,{isOpen:Re,onClose:()=>Me(!1),...Ne}),r.jsx("footer",{className:"py-5 text-xs mt-8",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",background:"var(--md-sys-color-surface-container-low)",color:"var(--md-sys-color-on-surface-variant)"},children:r.jsxs("div",{className:"max-w-[1440px] mx-auto px-4 sm:px-6 flex flex-col sm:flex-row items-center justify-between gap-2",children:[r.jsxs("div",{children:[r.jsx("span",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"CFAE System Framework v4.0.0"})," · ","Material Design 3 Autonomous Mutation Safety Console"]}),r.jsx("div",{className:"text-[11px] font-mono",children:"ISO C++23 · Rust 2024 · Kotlin 2.1 · CFJ4 Readback Verified Recovery"})]})})]})};class E0 extends Q.Component{constructor(){super(...arguments);Hc(this,"state",{hasError:!1,error:null});Hc(this,"handleReset",()=>{this.setState({hasError:!1,error:null}),window.location.reload()})}static getDerivedStateFromError(m){return{hasError:!0,error:m}}componentDidCatch(m,d){console.error("[CoreFlow M3 WebUI] Uncaught component error:",m,d)}render(){return this.state.hasError?r.jsx("div",{className:"min-h-screen flex items-center justify-center p-4",style:{backgroundColor:"var(--md-sys-color-background)",color:"var(--md-sys-color-on-surface)"},children:r.jsxs("div",{className:"md3-card-elevated max-w-md w-full p-7 text-center space-y-4",children:[r.jsx("div",{className:"w-12 h-12 rounded-2xl flex items-center justify-center mx-auto",style:{backgroundColor:"var(--md-sys-color-error-container)",color:"var(--md-sys-color-on-error-container)"},children:r.jsx(Cg,{className:"w-6 h-6"})}),r.jsxs("div",{className:"space-y-1.5",children:[r.jsx("h2",{className:"text-xl font-bold tracking-tight",children:"UI Surface Interrupted"}),r.jsxs("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["The Material 3 console caught a rendering exception. Native ",r.jsx("code",{className:"font-mono",children:"coreflowd"})," daemon state and OEM baseline journals remain untouched."]})]}),this.state.error&&r.jsx("pre",{className:"md3-card-filled p-3 text-[11px] font-mono text-left overflow-auto max-h-32",children:this.state.error.message}),r.jsx("div",{className:"pt-1 flex items-center justify-center gap-3",children:r.jsxs("button",{type:"button",id:"btn-error-reload",onClick:this.handleReset,className:"md3-btn-filled",children:[r.jsx(ip,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Reload Console"})]})}),r.jsxs("div",{className:"pt-2 flex items-center justify-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(On,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Write-ahead CFJ4 recovery remains independent"})]})]})}):this.props.children}}Sh.createRoot(document.getElementById("root")).render(r.jsx(Q.StrictMode,{children:r.jsx(E0,{children:r.jsx(j0,{})})}));
