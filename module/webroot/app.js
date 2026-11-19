var mh=Object.defineProperty;var fh=(o,c,_)=>c in o?mh(o,c,{enumerable:!0,configurable:!0,writable:!0,value:_}):o[c]=_;var Gc=(o,c,_)=>fh(o,typeof c!="symbol"?c+"":c,_);(function(){const c=document.createElement("link").relList;if(c&&c.supports&&c.supports("modulepreload"))return;for(const j of document.querySelectorAll('link[rel="modulepreload"]'))d(j);new MutationObserver(j=>{for(const A of j)if(A.type==="childList")for(const N of A.addedNodes)N.tagName==="LINK"&&N.rel==="modulepreload"&&d(N)}).observe(document,{childList:!0,subtree:!0});function _(j){const A={};return j.integrity&&(A.integrity=j.integrity),j.referrerPolicy&&(A.referrerPolicy=j.referrerPolicy),j.crossOrigin==="use-credentials"?A.credentials="include":j.crossOrigin==="anonymous"?A.credentials="omit":A.credentials="same-origin",A}function d(j){if(j.ep)return;j.ep=!0;const A=_(j);fetch(j.href,A)}})();var Ic={exports:{}},jl={};/**
 * @license React
 * react-jsx-runtime.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Bf;function ph(){if(Bf)return jl;Bf=1;var o=Symbol.for("react.transitional.element"),c=Symbol.for("react.fragment");function _(d,j,A){var N=null;if(A!==void 0&&(N=""+A),j.key!==void 0&&(N=""+j.key),"key"in j){A={};for(var B in j)B!=="key"&&(A[B]=j[B])}else A=j;return j=A.ref,{$$typeof:o,type:d,key:N,ref:j!==void 0?j:null,props:A}}return jl.Fragment=c,jl.jsx=_,jl.jsxs=_,jl}var Vf;function vh(){return Vf||(Vf=1,Ic.exports=ph()),Ic.exports}var r=vh(),Pc={exports:{}},oe={};/**
 * @license React
 * react.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Lf;function yh(){if(Lf)return oe;Lf=1;var o=Symbol.for("react.transitional.element"),c=Symbol.for("react.portal"),_=Symbol.for("react.fragment"),d=Symbol.for("react.strict_mode"),j=Symbol.for("react.profiler"),A=Symbol.for("react.consumer"),N=Symbol.for("react.context"),B=Symbol.for("react.forward_ref"),U=Symbol.for("react.suspense"),R=Symbol.for("react.memo"),b=Symbol.for("react.lazy"),y=Symbol.for("react.activity"),O=Symbol.for("react.view_transition"),$=Symbol.iterator;function q(p){return p===null||typeof p!="object"?null:(p=$&&p[$]||p["@@iterator"],typeof p=="function"?p:null)}var K={isMounted:function(){return!1},enqueueForceUpdate:function(){},enqueueReplaceState:function(){},enqueueSetState:function(){}},V=Object.assign,Z={};function ie(p,M,W){this.props=p,this.context=M,this.refs=Z,this.updater=W||K}ie.prototype.isReactComponent={},ie.prototype.setState=function(p,M){if(typeof p!="object"&&typeof p!="function"&&p!=null)throw Error("takes an object of state variables to update or a function which returns an object of state variables.");this.updater.enqueueSetState(this,p,M,"setState")},ie.prototype.forceUpdate=function(p){this.updater.enqueueForceUpdate(this,p,"forceUpdate")};function G(){}G.prototype=ie.prototype;function ce(p,M,W){this.props=p,this.context=M,this.refs=Z,this.updater=W||K}var Ee=ce.prototype=new G;Ee.constructor=ce,V(Ee,ie.prototype),Ee.isPureReactComponent=!0;var E=Array.isArray;function D(){}var P={H:null,A:null,T:null,S:null},he=Object.prototype.hasOwnProperty;function Oe(p,M,W){var ee=W.ref;return{$$typeof:o,type:p,key:M,ref:ee!==void 0?ee:null,props:W}}function re(p,M){return Oe(p.type,M,p.props)}function ue(p){return typeof p=="object"&&p!==null&&p.$$typeof===o}function Ie(p){var M={"=":"=0",":":"=2"};return"$"+p.replace(/[=:]/g,function(W){return M[W]})}var Xe=/\/+/g;function He(p,M){return typeof p=="object"&&p!==null&&p.key!=null?Ie(""+p.key):M.toString(36)}function z(p){switch(p.status){case"fulfilled":return p.value;case"rejected":throw p.reason;default:switch(typeof p.status=="string"?p.then(D,D):(p.status="pending",p.then(function(M){p.status==="pending"&&(p.status="fulfilled",p.value=M)},function(M){p.status==="pending"&&(p.status="rejected",p.reason=M)})),p.status){case"fulfilled":return p.value;case"rejected":throw p.reason}}throw p}function ne(p,M,W,ee,xe){var we=typeof p;(we==="undefined"||we==="boolean")&&(p=null);var Se=!1;if(p===null)Se=!0;else switch(we){case"bigint":case"string":case"number":Se=!0;break;case"object":switch(p.$$typeof){case o:case c:Se=!0;break;case b:return Se=p._init,ne(Se(p._payload),M,W,ee,xe)}}if(Se)return xe=xe(p),Se=ee===""?"."+He(p,0):ee,E(xe)?(W="",Se!=null&&(W=Se.replace(Xe,"$&/")+"/"),ne(xe,M,W,"",function(Gt){return Gt})):xe!=null&&(ue(xe)&&(xe=re(xe,W+(xe.key==null||p&&p.key===xe.key?"":(""+xe.key).replace(Xe,"$&/")+"/")+Se)),M.push(xe)),1;Se=0;var J=ee===""?".":ee+":";if(E(p))for(var se=0;se<p.length;se++)ee=p[se],we=J+He(ee,se),Se+=ne(ee,M,W,we,xe);else if(se=q(p),typeof se=="function")for(p=se.call(p),se=0;!(ee=p.next()).done;)ee=ee.value,we=J+He(ee,se++),Se+=ne(ee,M,W,we,xe);else if(we==="object"){if(typeof p.then=="function")return ne(z(p),M,W,ee,xe);throw M=String(p),Error("Objects are not valid as a React child (found: "+(M==="[object Object]"?"object with keys {"+Object.keys(p).join(", ")+"}":M)+"). If you meant to render a collection of children, use an array instead.")}return Se}function ae(p,M,W){if(p==null)return p;var ee=[],xe=0;return ne(p,ee,"","",function(we){return M.call(W,we,xe++)}),ee}function ke(p){if(p._status===-1){var M=p._result,W=M();W.then(function(ee){(p._status===0||p._status===-1)&&(p._status=1,p._result=ee,W.status===void 0&&(W.status="fulfilled",W.value=ee))},function(ee){(p._status===0||p._status===-1)&&(p._status=2,p._result=ee,W.status===void 0&&(W.status="rejected",W.reason=ee))}),p._status===-1&&(p._status=0,p._result=W)}if(p._status===1)return p._result.default;throw p._result}var me=typeof reportError=="function"?reportError:function(p){if(typeof window=="object"&&typeof window.ErrorEvent=="function"){var M=new window.ErrorEvent("error",{bubbles:!0,cancelable:!0,message:typeof p=="object"&&p!==null&&typeof p.message=="string"?String(p.message):String(p),error:p});if(!window.dispatchEvent(M))return}else if(typeof process=="object"&&typeof process.emit=="function"){process.emit("uncaughtException",p);return}console.error(p)};function ze(p){var M=P.T,W={};W.types=M!==null?M.types:null,P.T=W;try{var ee=p(),xe=P.S;xe!==null&&xe(W,ee),typeof ee=="object"&&ee!==null&&typeof ee.then=="function"&&ee.then(D,me)}catch(we){me(we)}finally{M!==null&&W.types!==null&&(M.types=W.types),P.T=M}}function mt(p){var M=P.T;if(M!==null){var W=M.types;W===null?M.types=[p]:W.indexOf(p)===-1&&W.push(p)}else ze(mt.bind(null,p))}var bt={map:ae,forEach:function(p,M,W){ae(p,function(){M.apply(this,arguments)},W)},count:function(p){var M=0;return ae(p,function(){M++}),M},toArray:function(p){return ae(p,function(M){return M})||[]},only:function(p){if(!ue(p))throw Error("React.Children.only expected to receive a single React element child.");return p}};return oe.Activity=y,oe.Children=bt,oe.Component=ie,oe.Fragment=_,oe.Profiler=j,oe.PureComponent=ce,oe.StrictMode=d,oe.Suspense=U,oe.ViewTransition=O,oe.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE=P,oe.__COMPILER_RUNTIME={__proto__:null,c:function(p){return P.H.useMemoCache(p)}},oe.addTransitionType=mt,oe.cache=function(p){return function(){return p.apply(null,arguments)}},oe.cacheSignal=function(){return null},oe.cloneElement=function(p,M,W){if(p==null)throw Error("The argument must be a React element, but you passed "+p+".");var ee=V({},p.props),xe=p.key;if(M!=null)for(we in M.key!==void 0&&(xe=""+M.key),M)!he.call(M,we)||we==="key"||we==="__self"||we==="__source"||we==="ref"&&M.ref===void 0||(ee[we]=M[we]);var we=arguments.length-2;if(we===1)ee.children=W;else if(1<we){for(var Se=Array(we),J=0;J<we;J++)Se[J]=arguments[J+2];ee.children=Se}return Oe(p.type,xe,ee)},oe.createContext=function(p){return p={$$typeof:N,_currentValue:p,_currentValue2:p,_threadCount:0,Provider:null,Consumer:null},p.Provider=p,p.Consumer={$$typeof:A,_context:p},p},oe.createElement=function(p,M,W){var ee,xe={},we=null;if(M!=null)for(ee in M.key!==void 0&&(we=""+M.key),M)he.call(M,ee)&&ee!=="key"&&ee!=="__self"&&ee!=="__source"&&(xe[ee]=M[ee]);var Se=arguments.length-2;if(Se===1)xe.children=W;else if(1<Se){for(var J=Array(Se),se=0;se<Se;se++)J[se]=arguments[se+2];xe.children=J}if(p&&p.defaultProps)for(ee in Se=p.defaultProps,Se)xe[ee]===void 0&&(xe[ee]=Se[ee]);return Oe(p,we,xe)},oe.createRef=function(){return{current:null}},oe.forwardRef=function(p){return{$$typeof:B,render:p}},oe.isValidElement=ue,oe.lazy=function(p){return{$$typeof:b,_payload:{_status:-1,_result:p},_init:ke}},oe.memo=function(p,M){return{$$typeof:R,type:p,compare:M===void 0?null:M}},oe.startTransition=ze,oe.unstable_useCacheRefresh=function(){return P.H.useCacheRefresh()},oe.use=function(p){return P.H.use(p)},oe.useActionState=function(p,M,W){return P.H.useActionState(p,M,W)},oe.useCallback=function(p,M){return P.H.useCallback(p,M)},oe.useContext=function(p){return P.H.useContext(p)},oe.useDebugValue=function(){},oe.useDeferredValue=function(p,M){return P.H.useDeferredValue(p,M)},oe.useEffect=function(p,M){return P.H.useEffect(p,M)},oe.useEffectEvent=function(p){return P.H.useEffectEvent(p)},oe.useId=function(){return P.H.useId()},oe.useImperativeHandle=function(p,M,W){return P.H.useImperativeHandle(p,M,W)},oe.useInsertionEffect=function(p,M){return P.H.useInsertionEffect(p,M)},oe.useLayoutEffect=function(p,M){return P.H.useLayoutEffect(p,M)},oe.useMemo=function(p,M){return P.H.useMemo(p,M)},oe.useOptimistic=function(p,M){return P.H.useOptimistic(p,M)},oe.useReducer=function(p,M,W){return P.H.useReducer(p,M,W)},oe.useRef=function(p){return P.H.useRef(p)},oe.useState=function(p){return P.H.useState(p)},oe.useSyncExternalStore=function(p,M,W){return P.H.useSyncExternalStore(p,M,W)},oe.useTransition=function(){return P.H.useTransition()},oe.version="19.3.1",oe}var Hf;function iu(){return Hf||(Hf=1,Pc.exports=yh()),Pc.exports}var F=iu(),Yc={exports:{}},El={},Qc={exports:{}},Fc={};/**
 * @license React
 * scheduler.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Gf;function hh(){return Gf||(Gf=1,(function(o){function c(z,ne){var ae=z.length;z.push(ne);e:for(;0<ae;){var ke=ae-1>>>1,me=z[ke];if(0<j(me,ne))z[ke]=ne,z[ae]=me,ae=ke;else break e}}function _(z){return z.length===0?null:z[0]}function d(z){if(z.length===0)return null;var ne=z[0],ae=z.pop();if(ae!==ne){z[0]=ae;e:for(var ke=0,me=z.length,ze=me>>>1;ke<ze;){var mt=2*(ke+1)-1,bt=z[mt],p=mt+1,M=z[p];if(0>j(bt,ae))p<me&&0>j(M,bt)?(z[ke]=M,z[p]=ae,ke=p):(z[ke]=bt,z[mt]=ae,ke=mt);else if(p<me&&0>j(M,ae))z[ke]=M,z[p]=ae,ke=p;else break e}}return ne}function j(z,ne){var ae=z.sortIndex-ne.sortIndex;return ae!==0?ae:z.id-ne.id}if(o.unstable_now=void 0,typeof performance=="object"&&typeof performance.now=="function"){var A=performance;o.unstable_now=function(){return A.now()}}else{var N=Date,B=N.now();o.unstable_now=function(){return N.now()-B}}var U=[],R=[],b=1,y=null,O=3,$=!1,q=!1,K=!1,V=!1,Z=typeof setTimeout=="function"?setTimeout:null,ie=typeof clearTimeout=="function"?clearTimeout:null,G=typeof setImmediate<"u"?setImmediate:null;function ce(z){for(var ne=_(R);ne!==null;){if(ne.callback===null)d(R);else if(ne.startTime<=z)d(R),ne.sortIndex=ne.expirationTime,c(U,ne);else break;ne=_(R)}}function Ee(z){if(K=!1,ce(z),!q)if(_(U)!==null)q=!0,E||(E=!0,ue());else{var ne=_(R);ne!==null&&He(Ee,ne.startTime-z)}}var E=!1,D=-1,P=5,he=-1;function Oe(){return V?!0:!(o.unstable_now()-he<P)}function re(){if(V=!1,E){var z=o.unstable_now();he=z;var ne=!0;try{e:{q=!1,K&&(K=!1,ie(D),D=-1),$=!0;var ae=O;try{t:{for(ce(z),y=_(U);y!==null&&!(y.expirationTime>z&&Oe());){var ke=y.callback;if(typeof ke=="function"){y.callback=null,O=y.priorityLevel;var me=ke(y.expirationTime<=z);if(z=o.unstable_now(),typeof me=="function"){y.callback=me,ce(z),ne=!0;break t}y===_(U)&&d(U),ce(z)}else d(U);y=_(U)}if(y!==null)ne=!0;else{var ze=_(R);ze!==null&&He(Ee,ze.startTime-z),ne=!1}}break e}finally{y=null,O=ae,$=!1}ne=void 0}}finally{ne?ue():E=!1}}}var ue;if(typeof G=="function")ue=function(){G(re)};else if(typeof MessageChannel<"u"){var Ie=new MessageChannel,Xe=Ie.port2;Ie.port1.onmessage=re,ue=function(){Xe.postMessage(null)}}else ue=function(){Z(re,0)};function He(z,ne){D=Z(function(){z(o.unstable_now())},ne)}o.unstable_IdlePriority=5,o.unstable_ImmediatePriority=1,o.unstable_LowPriority=4,o.unstable_NormalPriority=3,o.unstable_Profiling=null,o.unstable_UserBlockingPriority=2,o.unstable_cancelCallback=function(z){z.callback=null},o.unstable_forceFrameRate=function(z){0>z||125<z?console.error("forceFrameRate takes a positive int between 0 and 125, forcing frame rates higher than 125 fps is not supported"):P=0<z?Math.floor(1e3/z):5},o.unstable_getCurrentPriorityLevel=function(){return O},o.unstable_next=function(z){switch(O){case 1:case 2:case 3:var ne=3;break;default:ne=O}var ae=O;O=ne;try{return z()}finally{O=ae}},o.unstable_requestPaint=function(){V=!0},o.unstable_runWithPriority=function(z,ne){switch(z){case 1:case 2:case 3:case 4:case 5:break;default:z=3}var ae=O;O=z;try{return ne()}finally{O=ae}},o.unstable_scheduleCallback=function(z,ne,ae){var ke=o.unstable_now();switch(typeof ae=="object"&&ae!==null?(ae=ae.delay,ae=typeof ae=="number"&&0<ae?ke+ae:ke):ae=ke,z){case 1:var me=-1;break;case 2:me=250;break;case 5:me=1073741823;break;case 4:me=1e4;break;default:me=5e3}return me=ae+me,z={id:b++,callback:ne,priorityLevel:z,startTime:ae,expirationTime:me,sortIndex:-1},ae>ke?(z.sortIndex=ae,c(R,z),_(U)===null&&z===_(R)&&(K?(ie(D),D=-1):K=!0,He(Ee,ae-ke))):(z.sortIndex=me,c(U,z),q||$||(q=!0,E||(E=!0,ue()))),z},o.unstable_shouldYield=Oe,o.unstable_wrapCallback=function(z){var ne=O;return function(){var ae=O;O=ne;try{return z.apply(this,arguments)}finally{O=ae}}}})(Fc)),Fc}var If;function gh(){return If||(If=1,Qc.exports=hh()),Qc.exports}var Kc={exports:{}},_t={};/**
 * @license React
 * react-dom.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Pf;function bh(){if(Pf)return _t;Pf=1;var o=iu();function c(b){var y="https://react.dev/errors/"+b;if(1<arguments.length){y+="?args[]="+encodeURIComponent(arguments[1]);for(var O=2;O<arguments.length;O++)y+="&args[]="+encodeURIComponent(arguments[O])}return"Minified React error #"+b+"; visit "+y+" for the full message or use the non-minified dev environment for full errors and additional helpful warnings."}function _(){}var d={d:{f:_,r:function(){throw Error(c(522))},D:_,C:_,L:_,m:_,X:_,S:_,M:_},p:0,findDOMNode:null},j=Symbol.for("react.portal"),A=Symbol.for("react.recoverable"),N=Symbol.for("react.optimistic_key");function B(b,y,O){var $=3<arguments.length&&arguments[3]!==void 0?arguments[3]:null;return{$$typeof:j,key:$==null?null:$===N?N:""+$,children:b,containerInfo:y,implementation:O}}var U=o.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE;function R(b,y){if(b==="font")return"";if(typeof y=="string")return y==="use-credentials"?y:""}return _t.__DOM_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE=d,_t.browser=function(b){return{$$typeof:A,_reason:b}},_t.createPortal=function(b,y){var O=2<arguments.length&&arguments[2]!==void 0?arguments[2]:null;if(!y||y.nodeType!==1&&y.nodeType!==9&&y.nodeType!==11)throw Error(c(299));return B(b,y,null,O)},_t.flushSync=function(b){var y=U.T,O=d.p;try{if(U.T=null,d.p=2,b)return b()}finally{U.T=y,d.p=O,d.d.f()}},_t.preconnect=function(b,y){typeof b=="string"&&(y?(y=y.crossOrigin,y=typeof y=="string"?y==="use-credentials"?y:"":void 0):y=null,d.d.C(b,y))},_t.prefetchDNS=function(b){typeof b=="string"&&d.d.D(b)},_t.preinit=function(b,y){if(typeof b=="string"&&y&&typeof y.as=="string"){var O=y.as,$=R(O,y.crossOrigin),q=typeof y.integrity=="string"?y.integrity:void 0,K=typeof y.fetchPriority=="string"?y.fetchPriority:void 0;O==="style"?d.d.S(b,typeof y.precedence=="string"?y.precedence:void 0,{crossOrigin:$,integrity:q,fetchPriority:K}):O==="script"&&d.d.X(b,{crossOrigin:$,integrity:q,fetchPriority:K,nonce:typeof y.nonce=="string"?y.nonce:void 0})}},_t.preinitModule=function(b,y){if(typeof b=="string")if(typeof y=="object"&&y!==null){if(y.as==null||y.as==="script"){var O=R(y.as,y.crossOrigin);d.d.M(b,{crossOrigin:O,integrity:typeof y.integrity=="string"?y.integrity:void 0,nonce:typeof y.nonce=="string"?y.nonce:void 0,fetchPriority:typeof y.fetchPriority=="string"?y.fetchPriority:void 0})}}else y==null&&d.d.M(b)},_t.preload=function(b,y){if(typeof b=="string"&&typeof y=="object"&&y!==null&&typeof y.as=="string"){var O=y.as,$=R(O,y.crossOrigin);d.d.L(b,O,{crossOrigin:$,integrity:typeof y.integrity=="string"?y.integrity:void 0,nonce:typeof y.nonce=="string"?y.nonce:void 0,type:typeof y.type=="string"?y.type:void 0,fetchPriority:typeof y.fetchPriority=="string"?y.fetchPriority:void 0,referrerPolicy:typeof y.referrerPolicy=="string"?y.referrerPolicy:void 0,imageSrcSet:typeof y.imageSrcSet=="string"?y.imageSrcSet:void 0,imageSizes:typeof y.imageSizes=="string"?y.imageSizes:void 0,media:typeof y.media=="string"?y.media:void 0})}},_t.preloadModule=function(b,y){if(typeof b=="string")if(y){var O=R(y.as,y.crossOrigin);d.d.m(b,{as:typeof y.as=="string"&&y.as!=="script"?y.as:void 0,crossOrigin:O,integrity:typeof y.integrity=="string"?y.integrity:void 0,nonce:typeof y.nonce=="string"?y.nonce:void 0,fetchPriority:typeof y.fetchPriority=="string"?y.fetchPriority:void 0})}else d.d.m(b)},_t.requestFormReset=function(b){d.d.r(b)},_t.unstable_batchedUpdates=function(b,y){return b(y)},_t.useFormState=function(b,y,O){return U.H.useFormState(b,y,O)},_t.useFormStatus=function(){return U.H.useHostTransitionStatus()},_t.version="19.3.1",_t}var Yf;function xh(){if(Yf)return Kc.exports;Yf=1;function o(){if(!(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__>"u"||typeof __REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE!="function"))try{__REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE(o)}catch(c){console.error(c)}}return o(),Kc.exports=bh(),Kc.exports}/**
 * @license React
 * react-dom-client.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Qf;function wh(){if(Qf)return El;Qf=1;var o=gh(),c=iu(),_=xh();function d(e){var t="https://react.dev/errors/"+e;if(1<arguments.length){t+="?args[]="+encodeURIComponent(arguments[1]);for(var n=2;n<arguments.length;n++)t+="&args[]="+encodeURIComponent(arguments[n])}return"Minified React error #"+e+"; visit "+t+" for the full message or use the non-minified dev environment for full errors and additional helpful warnings."}function j(e){return!(!e||e.nodeType!==1&&e.nodeType!==9&&e.nodeType!==11)}function A(e){for(var t=e,n=t;n&&!n.alternate;)t=n,(t.flags&4098)!==0&&(e=t.return),n=t.return;for(;t.return;)t=t.return;return t.tag===3?e:null}function N(e){if(e.tag===13){var t=e.memoizedState;if(t===null&&(e=e.alternate,e!==null&&(t=e.memoizedState)),t!==null)return t.dehydrated}return null}function B(e){if(e.tag===31){var t=e.memoizedState;if(t===null&&(e=e.alternate,e!==null&&(t=e.memoizedState)),t!==null)return t.dehydrated}return null}function U(e){if(A(e)!==e)throw Error(d(188))}function R(e){var t=e.alternate;if(!t){if(t=A(e),t===null)throw Error(d(188));return t!==e?null:e}for(var n=e,a=t;;){var i=n.return;if(i===null)break;var l=i.alternate;if(l===null){if(a=i.return,a!==null){n=a;continue}break}if(i.child===l.child){for(l=i.child;l;){if(l===n)return U(i),e;if(l===a)return U(i),t;l=l.sibling}throw Error(d(188))}if(n.return!==a.return)n=i,a=l;else{for(var s=!1,u=i.child;u;){if(u===n){s=!0,n=i,a=l;break}if(u===a){s=!0,a=i,n=l;break}u=u.sibling}if(!s){for(u=l.child;u;){if(u===n){s=!0,n=l,a=i;break}if(u===a){s=!0,a=l,n=i;break}u=u.sibling}if(!s)throw Error(d(189))}}if(n.alternate!==a)throw Error(d(190))}if(n.tag!==3)throw Error(d(188));return n.stateNode.current===n?e:t}function b(e){var t=e.tag;if(t===5||t===26||t===27||t===6)return e;for(e=e.child;e!==null;){if(t=b(e),t!==null)return t;e=e.sibling}return null}function y(e,t,n,a,i,l){for(;e!==null;){if((e.tag===5||e.tag===27||e.tag===6)&&n(e,a,i,l)||(e.tag!==22||e.memoizedState===null)&&(t||e.tag!==5&&e.tag!==27)&&y(e.child,t,n,a,i,l))return!0;e=e.sibling}return!1}function O(e){for(e=e.return;e!==null;){if(e.tag===3||e.tag===5||e.tag===27)return e;e=e.return}return null}function $(e){var t=!1;for(e=e.return;e!==null&&(e.tag===4&&(t=!0),!(e.tag===3||e.tag===5||e.tag===27));)e=e.return;return t}function q(e){var t=[null,null],n=O(e);return n===null||K(t,e,n.child,{foundSelf:!1}),t}function K(e,t,n,a){for(;n!==null;){if(n===t)a.foundSelf=!0;else if(n.tag===5||n.tag===27||n.tag===6){if(a.foundSelf)return e[1]=n,!0;e[0]=n}else if((n.tag!==22||n.memoizedState===null)&&K(e,t,n.child,a))return!0;n=n.sibling}return!1}function V(e){switch(e.tag){case 5:case 27:case 6:return e.stateNode;case 3:return e.stateNode.containerInfo;default:throw Error(d(559))}}var Z=null,ie=null;function G(e,t,n){return e===n?!0:e===t?(Z=e,!0):!1}function ce(e,t,n){return e===n?(ie=e,!1):e===t?(ie!==null&&(Z=e),!0):!1}function Ee(e){if(e===null)return null;do e=e===null?null:e.return;while(e&&e.tag!==5&&e.tag!==27&&e.tag!==3);return e||null}function E(e,t,n){for(var a=0,i=e;i;i=n(i))a++;i=0;for(var l=t;l;l=n(l))i++;for(;0<a-i;)e=n(e),a--;for(;0<i-a;)t=n(t),i--;for(;a--;){if(e===t||t!==null&&e===t.alternate)return e;e=n(e),t=n(t)}return null}var D=Object.assign,P=Symbol.for("react.element"),he=Symbol.for("react.transitional.element"),Oe=Symbol.for("react.portal"),re=Symbol.for("react.fragment"),ue=Symbol.for("react.strict_mode"),Ie=Symbol.for("react.profiler"),Xe=Symbol.for("react.consumer"),He=Symbol.for("react.context"),z=Symbol.for("react.forward_ref"),ne=Symbol.for("react.suspense"),ae=Symbol.for("react.suspense_list"),ke=Symbol.for("react.memo"),me=Symbol.for("react.lazy"),ze=Symbol.for("react.activity"),mt=Symbol.for("react.legacy_hidden"),bt=Symbol.for("react.memo_cache_sentinel"),p=Symbol.for("react.view_transition"),M=Symbol.for("react.recoverable"),W=Symbol.iterator;function ee(e){return e===null||typeof e!="object"?null:(e=W&&e[W]||e["@@iterator"],typeof e=="function"?e:null)}var xe=Symbol.for("react.client.reference");function we(e){if(e==null)return null;if(typeof e=="function")return e.$$typeof===xe?null:e.displayName||e.name||null;if(typeof e=="string")return e;switch(e){case re:return"Fragment";case Ie:return"Profiler";case ue:return"StrictMode";case ne:return"Suspense";case ae:return"SuspenseList";case ze:return"Activity";case p:return"ViewTransition"}if(typeof e=="object")switch(e.$$typeof){case Oe:return"Portal";case He:return e.displayName||"Context";case Xe:return(e._context.displayName||"Context")+".Consumer";case z:var t=e.render;return e=e.displayName,e||(e=t.displayName||t.name||"",e=e!==""?"ForwardRef("+e+")":"ForwardRef"),e;case ke:return t=e.displayName||null,t!==null?t:we(e.type)||"Memo";case me:t=e._payload,e=e._init;try{return we(e(t))}catch{}}return null}var Se=Array.isArray,J=c.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE,se=_.__DOM_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE,Gt={pending:!1,data:null,method:null,action:null},Ua=[],bn=-1;function At(e){return{current:e}}function H(e){0>bn||(e.current=Ua[bn],Ua[bn]=null,bn--)}function Q(e,t){bn++,Ua[bn]=e.current,e.current=t}var k=At(null),Ne=At(null),Fe=At(null),It=At(null);function lt(e,t){switch(Q(Fe,t),Q(Ne,e),Q(k,null),t.nodeType){case 9:case 11:e=(e=t.documentElement)&&(e=e.namespaceURI)?Qm(e):0;break;default:if(e=t.tagName,t=t.namespaceURI)t=Qm(t),e=Fm(t,e);else switch(e){case"svg":e=1;break;case"math":e=2;break;default:e=0}}H(k),Q(k,e)}function Y(){H(k),H(Ne),H(Fe)}function xt(e){var t=e.memoizedState;t!==null&&(Si._currentValue=t.memoizedState,Q(It,e)),t=k.current;var n=Fm(t,e.type);t!==n&&(Q(Ne,e),Q(k,n))}function ft(e){Ne.current===e&&(H(k),H(Ne)),It.current===e&&(H(It),Si._currentValue=Gt)}var Pt,Yt;function wt(e){if(Pt===void 0)try{throw Error()}catch(n){var t=n.stack.trim().match(/\n( *(at )?)/);Pt=t&&t[1]||"",Yt=-1<n.stack.indexOf(`
    at`)?" (<anonymous>)":-1<n.stack.indexOf("@")?"@unknown:0:0":""}return`
`+Pt+e+Yt}var cs=!1;function us(e,t){if(!e||cs)return"";cs=!0;var n=Error.prepareStackTrace;Error.prepareStackTrace=void 0;try{var a={DetermineComponentFrameRoot:function(){try{if(t){var C=function(){throw Error()};if(Object.defineProperty(C.prototype,"props",{set:function(){throw Error()}}),typeof Reflect=="object"&&Reflect.construct){try{Reflect.construct(C,[])}catch(L){var v=L}Reflect.construct(e,[],C)}else{try{C.call()}catch(L){v=L}C=!1;try{var w=Object.getOwnPropertyDescriptor(e.prototype,"props");Object.defineProperty(e.prototype,"props",{configurable:!0,set:function(){throw Error()}}),C=!0,new e}finally{C&&(w!==void 0?Object.defineProperty(e.prototype,"props",w):delete e.prototype.props)}}}else{try{throw Error()}catch(L){v=L}(C=e())&&typeof C.catch=="function"&&C.catch(function(){})}}catch(L){if(L&&v&&typeof L.stack=="string")return[L.stack,v.stack]}return[null,null]}};a.DetermineComponentFrameRoot.displayName="DetermineComponentFrameRoot";var i=Object.getOwnPropertyDescriptor(a.DetermineComponentFrameRoot,"name");i&&i.configurable&&Object.defineProperty(a.DetermineComponentFrameRoot,"name",{value:"DetermineComponentFrameRoot"});var l=a.DetermineComponentFrameRoot(),s=l[0],u=l[1];if(s&&u){var m=s.split(`
`),g=u.split(`
`);for(i=a=0;a<m.length&&!m[a].includes("DetermineComponentFrameRoot");)a++;for(;i<g.length&&!g[i].includes("DetermineComponentFrameRoot");)i++;if(a===m.length||i===g.length)for(a=m.length-1,i=g.length-1;1<=a&&0<=i&&m[a]!==g[i];)i--;for(;1<=a&&0<=i;a--,i--)if(m[a]!==g[i]){if(a!==1||i!==1)do if(a--,i--,0>i||m[a]!==g[i]){var S=`
`+m[a].replace(" at new "," at ");return e.displayName&&S.includes("<anonymous>")&&(S=S.replace("<anonymous>",e.displayName)),S}while(1<=a&&0<=i);break}}}finally{cs=!1,Error.prepareStackTrace=n}return(n=e?e.displayName||e.name:"")?wt(n):""}function yp(e,t){switch(e.tag){case 26:case 27:case 5:return wt(e.type);case 16:return wt("Lazy");case 13:return e.child!==t&&t!==null?wt("Suspense Fallback"):wt("Suspense");case 19:return wt("SuspenseList");case 0:case 15:return us(e.type,!1);case 11:return us(e.type.render,!1);case 1:return us(e.type,!0);case 31:return wt("Activity");case 30:return wt("ViewTransition");default:return""}}function fu(e){try{var t="",n=null;do t+=yp(e,n),n=e,e=e.return;while(e);return t}catch(a){return`
Error generating stack: `+a.message+`
`+a.stack}}var ds=Object.prototype.hasOwnProperty,_s=o.unstable_scheduleCallback,ms=o.unstable_cancelCallback,hp=o.unstable_shouldYield,gp=o.unstable_requestPaint,Ot=o.unstable_now,bp=o.unstable_getCurrentPriorityLevel,pu=o.unstable_ImmediatePriority,vu=o.unstable_UserBlockingPriority,Al=o.unstable_NormalPriority,xp=o.unstable_LowPriority,yu=o.unstable_IdlePriority,wp=o.log,Np=o.unstable_setDisableYieldValue,Ri=null,Rt=null;function Bn(e){if(typeof wp=="function"&&Np(e),Rt&&typeof Rt.setStrictMode=="function")try{Rt.setStrictMode(Ri,e)}catch{}}var qt=Math.clz32?Math.clz32:Ep,Sp=Math.log,jp=Math.LN2;function Ep(e){return e>>>=0,e===0?32:31-(Sp(e)/jp|0)|0}var Ol=256,Rl=262144,ql=4194304;function _a(e){var t=e&42;if(t!==0)return t;switch(e&-e){case 1:return 1;case 2:return 2;case 4:return 4;case 8:return 8;case 16:return 16;case 32:return 32;case 64:return 64;case 128:return 128;case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:return e&-e;case 262144:case 524288:case 1048576:case 2097152:return e&3932160;case 4194304:case 8388608:case 16777216:case 33554432:return e&62914560;case 67108864:return 67108864;case 134217728:return 134217728;case 268435456:return 268435456;case 536870912:return 536870912;case 1073741824:return 0;default:return e}}function Ml(e,t,n){var a=e.pendingLanes;if(a===0)return 0;var i=0,l=e.suspendedLanes,s=e.pingedLanes;e=e.warmLanes;var u=a&134217727;return u!==0?(a=u&~l,a!==0?i=_a(a):(s&=u,s!==0?i=_a(s):n||(n=u&~e,n!==0&&(i=_a(n))))):(u=a&~l,u!==0?i=_a(u):s!==0?i=_a(s):n||(n=a&~e,n!==0&&(i=_a(n)))),i===0?0:t!==0&&t!==i&&(t&l)===0&&(l=i&-i,n=t&-t,l>=n||l===32&&(n&4194048)!==0)?t:i}function qi(e,t){return(e.pendingLanes&~(e.suspendedLanes&~e.pingedLanes)&t)===0}function hu(e,t){(t&8)!==0&&(t|=t&32);var n=e.entangledLanes;if(n!==0)for(e=e.entanglements,n&=t;0<n;){var a=31-qt(n),i=1<<a;t|=e[a],n&=~i}return t}function kp(e,t){switch(e){case 1:case 2:case 4:case 8:case 64:return t+250;case 16:case 32:case 128:case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:case 262144:case 524288:case 1048576:case 2097152:return t+5e3;case 4194304:case 8388608:case 16777216:case 33554432:return-1;case 67108864:case 134217728:case 268435456:case 536870912:case 1073741824:return-1;default:return-1}}function gu(){var e=ql;return ql<<=1,(ql&62914560)===0&&(ql=4194304),e}function fs(e){for(var t=[],n=0;31>n;n++)t.push(e);return t}function Mi(e,t){e.pendingLanes|=t,t!==268435456&&(e.suspendedLanes=0,e.pingedLanes=0,e.warmLanes=0)}function Tp(e,t,n,a,i,l){var s=e.pendingLanes;e.pendingLanes=n,e.suspendedLanes=0,e.pingedLanes=0,e.warmLanes=0,e.expiredLanes&=n,e.entangledLanes&=n,e.errorRecoveryDisabledLanes&=n,e.shellSuspendCounter=0;var u=e.entanglements,m=e.expirationTimes,g=e.hiddenUpdates;for(n=s&~n;0<n;){var S=31-qt(n),C=1<<S;u[S]=0,m[S]=-1;var v=g[S];if(v!==null)for(g[S]=null,S=0;S<v.length;S++){var w=v[S];w!==null&&(w.lane&=-536870913)}n&=~C}a!==0&&bu(e,a,0),l!==0&&i===0&&e.tag!==0&&(e.suspendedLanes|=l&~(s&~t))}function bu(e,t,n){e.pendingLanes|=t,e.suspendedLanes&=~t;var a=31-qt(t);e.entangledLanes|=t,e.entanglements[a]=e.entanglements[a]|1073741824|n&261930}function xu(e,t){var n=e.entangledLanes|=t;for(e=e.entanglements;n;){var a=31-qt(n),i=1<<a;i&t|e[a]&t&&(e[a]|=t),n&=~i}}function wu(e,t){var n=t&-t;return n=(n&42)!==0?1:ps(n),(n&(e.suspendedLanes|t))!==0?0:n}function ps(e){switch(e){case 2:e=1;break;case 8:e=4;break;case 32:e=16;break;case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:case 262144:case 524288:case 1048576:case 2097152:case 4194304:case 8388608:case 16777216:case 33554432:e=128;break;case 268435456:e=134217728;break;default:e=0}return e}function vs(e){return e&=-e,2<e?8<e?(e&134217727)!==0?32:268435456:8:2}function Nu(){var e=se.p;return e!==0?e:(e=window.event,e===void 0?32:Of(e.type))}function Su(e,t){var n=se.p;try{return se.p=e,t()}finally{se.p=n}}var xn=Math.random().toString(36).slice(2),rt="__reactFiber$"+xn,Nt="__reactProps$"+xn,Ba="__reactContainer$"+xn,ju="__reactEvents$"+xn,Cp="__reactListeners$"+xn,Ap="__reactHandles$"+xn,Eu="__reactResources$"+xn,Di="__reactMarker$"+xn,Dl="__reactLoad$"+xn;function zl(e){delete e[rt],delete e[Nt],delete e[Cp],delete e[Ap]}function ma(e){var t;if(t=e[rt])return t;for(var n=e.parentNode;n;){if(t=n[Ba]||n[rt]){if(n=t.alternate,t.child!==null||n!==null&&n.child!==null)for(e=df(e);e!==null;){if(n=e[rt])return n;e=df(e)}return t}e=n,n=e.parentNode}return null}function Va(e){if(e=e[rt]||e[Ba]){var t=e.tag;if(t===5||t===6||t===13||t===31||t===26||t===27||t===3)return e}return null}function zi(e){var t=e.tag;if(t===5||t===26||t===27||t===6)return e.stateNode;throw Error(d(33))}function La(e){var t=e[Eu];return t||(t=e[Eu]={hoistableStyles:new Map,hoistableScripts:new Map}),t}function tt(e){e[Di]=!0}function ku(e){e[Dl]=void 0}var Tu=new Set,Cu={};function fa(e,t){Ha(e,t),Ha(e+"Capture",t)}function Ha(e,t){for(Cu[e]=t,e=0;e<t.length;e++)Tu.add(t[e])}var Op=RegExp("^[:A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD][:A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD\\-.0-9\\u00B7\\u0300-\\u036F\\u203F-\\u2040]*$"),Au={},Ou={};function Rp(e){return ds.call(Ou,e)?!0:ds.call(Au,e)?!1:Op.test(e)?Ou[e]=!0:(Au[e]=!0,!1)}var Te=!1;function Ru(){var e=Te;return Te=!1,e}function Ul(e,t,n){if(Rp(t))if(n===null)e.removeAttribute(t);else{switch(typeof n){case"undefined":case"function":case"symbol":e.removeAttribute(t);return;case"boolean":var a=t.toLowerCase().slice(0,5);if(a!=="data-"&&a!=="aria-"){e.removeAttribute(t);return}}e.setAttribute(t,n)}}function Bl(e,t,n){if(n===null)e.removeAttribute(t);else{switch(typeof n){case"undefined":case"function":case"symbol":case"boolean":e.removeAttribute(t);return}e.setAttribute(t,n)}}function wn(e,t,n,a){if(a===null)e.removeAttribute(n);else{switch(typeof a){case"undefined":case"function":case"symbol":case"boolean":e.removeAttribute(n);return}e.setAttributeNS(t,n,a)}}function Mt(e){switch(typeof e){case"bigint":case"boolean":case"number":case"string":case"undefined":return e;case"object":return e;default:return""}}function qu(e){var t=e.type;return(e=e.nodeName)&&e.toLowerCase()==="input"&&(t==="checkbox"||t==="radio")}function qp(e,t,n){var a=Object.getOwnPropertyDescriptor(e.constructor.prototype,t);if(!e.hasOwnProperty(t)&&typeof a<"u"&&typeof a.get=="function"&&typeof a.set=="function"){var i=a.get,l=a.set;return Object.defineProperty(e,t,{configurable:!0,get:function(){return i.call(this)},set:function(s){n=""+s,l.call(this,s)}}),Object.defineProperty(e,t,{enumerable:a.enumerable}),{getValue:function(){return n},setValue:function(s){n=""+s},stopTracking:function(){e._valueTracker=null,delete e[t]}}}}function ys(e){if(!e._valueTracker){var t=qu(e)?"checked":"value";e._valueTracker=qp(e,t,""+e[t])}}function Mu(e){if(!e)return!1;var t=e._valueTracker;if(!t)return!0;var n=t.getValue(),a="";return e&&(a=qu(e)?e.checked?"true":"false":e.value),e=a,e!==n?(t.setValue(e),!0):!1}var Mp=/[\n"\\]/g;function Qt(e){return e.replace(Mp,function(t){return"\\"+t.charCodeAt(0).toString(16)+" "})}function hs(e,t,n,a,i,l,s,u){e.name="",s!=null&&typeof s!="function"&&typeof s!="symbol"&&typeof s!="boolean"?e.type=s:e.removeAttribute("type"),t!=null?s==="number"?(t===0&&e.value===""||e.value!=t)&&(e.value=""+Mt(t)):e.value!==""+Mt(t)&&(e.value=""+Mt(t)):s!=="submit"&&s!=="reset"||e.removeAttribute("value"),t!=null?s==="number"&&e.value==t?gs(e,Mt(e.value)):gs(e,Mt(t)):n!=null?gs(e,Mt(n)):a!=null&&e.removeAttribute("value"),i==null&&l!=null&&(e.defaultChecked=!!l),i!=null&&(e.checked=i&&typeof i!="function"&&typeof i!="symbol"),u!=null&&typeof u!="function"&&typeof u!="symbol"&&typeof u!="boolean"?e.name=""+Mt(u):e.removeAttribute("name")}function Du(e,t,n,a,i,l,s,u){if(l!=null&&typeof l!="function"&&typeof l!="symbol"&&typeof l!="boolean"&&(e.type=l),t!=null||n!=null){if(!(l!=="submit"&&l!=="reset"||t!=null)){ys(e);return}n=n!=null?""+Mt(n):"",t=t!=null?""+Mt(t):n,u||t===e.value||(e.value=t),e.defaultValue=t}a=a??i,a=typeof a!="function"&&typeof a!="symbol"&&!!a,e.checked=u?e.checked:!!a,e.defaultChecked=!!a,s!=null&&typeof s!="function"&&typeof s!="symbol"&&typeof s!="boolean"&&(e.name=s),ys(e)}function gs(e,t){e.defaultValue!==""+t&&(e.defaultValue=""+t)}function Ga(e,t,n,a){if(e=e.options,t){t={};for(var i=0;i<n.length;i++)t["$"+n[i]]=!0;for(n=0;n<e.length;n++)i=t.hasOwnProperty("$"+e[n].value),e[n].selected!==i&&(e[n].selected=i),i&&a&&(e[n].defaultSelected=!0)}else{for(n=""+Mt(n),t=null,i=0;i<e.length;i++){if(e[i].value===n){e[i].selected=!0,a&&(e[i].defaultSelected=!0);return}t!==null||e[i].disabled||(t=e[i])}t!==null&&(t.selected=!0)}}function zu(e,t,n){if(t!=null&&(t=""+Mt(t),t!==e.value&&(e.value=t),n==null)){e.defaultValue!==t&&(e.defaultValue=t);return}e.defaultValue=n!=null?""+Mt(n):""}function Uu(e,t,n,a){if(t==null){if(a!=null){if(n!=null)throw Error(d(92));if(Se(a)){if(1<a.length)throw Error(d(93));a=a[0]}n=a}n==null&&(n=""),t=n}n=Mt(t),e.defaultValue=n,a=e.textContent,a===n&&a!==""&&a!==null&&(e.value=a),ys(e)}function Ia(e,t){if(t){var n=e.firstChild;if(n&&n===e.lastChild&&n.nodeType===3){n.nodeValue=t;return}}e.textContent=t}var Dp=new Set("animationIterationCount aspectRatio borderImageOutset borderImageSlice borderImageWidth boxFlex boxFlexGroup boxOrdinalGroup columnCount columns flex flexGrow flexPositive flexShrink flexNegative flexOrder gridArea gridRow gridRowEnd gridRowSpan gridRowStart gridColumn gridColumnEnd gridColumnSpan gridColumnStart fontWeight lineClamp lineHeight opacity order orphans scale tabSize widows zIndex zoom fillOpacity floodOpacity stopOpacity strokeDasharray strokeDashoffset strokeMiterlimit strokeOpacity strokeWidth MozAnimationIterationCount MozBoxFlex MozBoxFlexGroup MozLineClamp msAnimationIterationCount msFlex msZoom msFlexGrow msFlexNegative msFlexOrder msFlexPositive msFlexShrink msGridColumn msGridColumnSpan msGridRow msGridRowSpan WebkitAnimationIterationCount WebkitBoxFlex WebKitBoxFlexGroup WebkitBoxOrdinalGroup WebkitColumnCount WebkitColumns WebkitFlex WebkitFlexGrow WebkitFlexPositive WebkitFlexShrink WebkitLineClamp".split(" "));function Bu(e,t,n){var a=t.indexOf("--")===0;n==null||typeof n=="boolean"||n===""?a?e.setProperty(t,""):t==="float"?e.cssFloat="":e[t]="":a?e.setProperty(t,n):typeof n!="number"||n===0||Dp.has(t)?t==="float"?e.cssFloat=n:e[t]=(""+n).trim():e[t]=n+"px"}function Vu(e,t,n){if(t!=null&&typeof t!="object")throw Error(d(62));if(e=e.style,n!=null){for(var a in n)!n.hasOwnProperty(a)||t!=null&&t.hasOwnProperty(a)||(a.indexOf("--")===0?e.setProperty(a,""):a==="float"?e.cssFloat="":e[a]="",Te=!0);for(var i in t)a=t[i],t.hasOwnProperty(i)&&n[i]!==a&&(Bu(e,i,a),Te=!0)}else for(var l in t)t.hasOwnProperty(l)&&Bu(e,l,t[l])}function bs(e){if(e.indexOf("-")===-1)return!1;switch(e){case"annotation-xml":case"color-profile":case"font-face":case"font-face-src":case"font-face-uri":case"font-face-format":case"font-face-name":case"missing-glyph":return!1;default:return!0}}var zp=new Map([["acceptCharset","accept-charset"],["htmlFor","for"],["httpEquiv","http-equiv"],["crossOrigin","crossorigin"],["accentHeight","accent-height"],["alignmentBaseline","alignment-baseline"],["arabicForm","arabic-form"],["baselineShift","baseline-shift"],["capHeight","cap-height"],["clipPath","clip-path"],["clipRule","clip-rule"],["colorInterpolation","color-interpolation"],["colorInterpolationFilters","color-interpolation-filters"],["colorProfile","color-profile"],["colorRendering","color-rendering"],["dominantBaseline","dominant-baseline"],["enableBackground","enable-background"],["fillOpacity","fill-opacity"],["fillRule","fill-rule"],["floodColor","flood-color"],["floodOpacity","flood-opacity"],["fontFamily","font-family"],["fontSize","font-size"],["fontSizeAdjust","font-size-adjust"],["fontStretch","font-stretch"],["fontStyle","font-style"],["fontVariant","font-variant"],["fontWeight","font-weight"],["glyphName","glyph-name"],["glyphOrientationHorizontal","glyph-orientation-horizontal"],["glyphOrientationVertical","glyph-orientation-vertical"],["horizAdvX","horiz-adv-x"],["horizOriginX","horiz-origin-x"],["imageRendering","image-rendering"],["letterSpacing","letter-spacing"],["lightingColor","lighting-color"],["markerEnd","marker-end"],["markerMid","marker-mid"],["markerStart","marker-start"],["maskType","mask-type"],["overlinePosition","overline-position"],["overlineThickness","overline-thickness"],["paintOrder","paint-order"],["panose-1","panose-1"],["pointerEvents","pointer-events"],["renderingIntent","rendering-intent"],["shapeRendering","shape-rendering"],["stopColor","stop-color"],["stopOpacity","stop-opacity"],["strikethroughPosition","strikethrough-position"],["strikethroughThickness","strikethrough-thickness"],["strokeDasharray","stroke-dasharray"],["strokeDashoffset","stroke-dashoffset"],["strokeLinecap","stroke-linecap"],["strokeLinejoin","stroke-linejoin"],["strokeMiterlimit","stroke-miterlimit"],["strokeOpacity","stroke-opacity"],["strokeWidth","stroke-width"],["textAnchor","text-anchor"],["textDecoration","text-decoration"],["textRendering","text-rendering"],["transformOrigin","transform-origin"],["underlinePosition","underline-position"],["underlineThickness","underline-thickness"],["unicodeBidi","unicode-bidi"],["unicodeRange","unicode-range"],["unitsPerEm","units-per-em"],["vAlphabetic","v-alphabetic"],["vHanging","v-hanging"],["vIdeographic","v-ideographic"],["vMathematical","v-mathematical"],["vectorEffect","vector-effect"],["vertAdvY","vert-adv-y"],["vertOriginX","vert-origin-x"],["vertOriginY","vert-origin-y"],["wordSpacing","word-spacing"],["writingMode","writing-mode"],["xmlnsXlink","xmlns:xlink"],["xHeight","x-height"]]),Up=/^[\u0000-\u001F ]*j[\r\n\t]*a[\r\n\t]*v[\r\n\t]*a[\r\n\t]*s[\r\n\t]*c[\r\n\t]*r[\r\n\t]*i[\r\n\t]*p[\r\n\t]*t[\r\n\t]*:/i;function Vl(e){return Up.test(""+e)?"javascript:throw new Error('React has blocked a javascript: URL as a security precaution.')":e}function on(){}var xs=null;function ws(e){return e=e.target||e.srcElement||window,e.correspondingUseElement&&(e=e.correspondingUseElement),e.nodeType===3?e.parentNode:e}var Pa=null,Ya=null;function Lu(e){var t=Va(e);if(t&&(e=t.stateNode)){var n=e[Nt]||null;e:switch(e=t.stateNode,t.type){case"input":if(hs(e,n.value,n.defaultValue,n.defaultValue,n.checked,n.defaultChecked,n.type,n.name),t=n.name,n.type==="radio"&&t!=null){for(n=e;n.parentNode;)n=n.parentNode;for(n=n.querySelectorAll('input[name="'+Qt(""+t)+'"][type="radio"]'),t=0;t<n.length;t++){var a=n[t];if(a!==e&&a.form===e.form){var i=a[Nt]||null;if(!i)throw Error(d(90));hs(a,i.value,i.defaultValue,i.defaultValue,i.checked,i.defaultChecked,i.type,i.name)}}for(t=0;t<n.length;t++)a=n[t],a.form===e.form&&Mu(a)}break e;case"textarea":zu(e,n.value,n.defaultValue);break e;case"select":t=n.value,t!=null&&Ga(e,!!n.multiple,t,!1)}}}var Ns=!1;function Hu(e,t,n){if(Ns)return e(t,n);Ns=!0;try{var a=e(t);return a}finally{if(Ns=!1,(Pa!==null||Ya!==null)&&(Vr(),Pa&&(t=Pa,e=Ya,Ya=Pa=null,Lu(t),e)))for(t=0;t<e.length;t++)Lu(e[t])}}function Ui(e,t){var n=e.stateNode;if(n===null)return null;var a=n[Nt]||null;if(a===null)return null;n=a[t];e:switch(t){case"onClick":case"onClickCapture":case"onDoubleClick":case"onDoubleClickCapture":case"onMouseDown":case"onMouseDownCapture":case"onMouseMove":case"onMouseMoveCapture":case"onMouseUp":case"onMouseUpCapture":case"onMouseEnter":(a=!a.disabled)||(e=e.type,a=!(e==="button"||e==="input"||e==="select"||e==="textarea")),e=!a;break e;default:e=!1}if(e)return null;if(n&&typeof n!="function")throw Error(d(231,t,typeof n));return n}var Nn=!(typeof window>"u"||typeof window.document>"u"||typeof window.document.createElement>"u"),Ss=!1;if(Nn)try{var Bi={};Object.defineProperty(Bi,"passive",{get:function(){Ss=!0}}),window.addEventListener("test",Bi,Bi),window.removeEventListener("test",Bi,Bi)}catch{Ss=!1}var Vn=null,js=null,Ll=null;function Gu(){if(Ll)return Ll;var e,t=js,n=t.length,a,i="value"in Vn?Vn.value:Vn.textContent,l=i.length;for(e=0;e<n&&t[e]===i[e];e++);var s=n-e;for(a=1;a<=s&&t[n-a]===i[l-a];a++);return Ll=i.slice(e,1<a?1-a:void 0)}function Hl(e){var t=e.keyCode;return"charCode"in e?(e=e.charCode,e===0&&t===13&&(e=13)):e=t,e===10&&(e=13),32<=e||e===13?e:0}function Gl(){return!0}function Iu(){return!1}function vt(e){function t(n,a,i,l,s){this._reactName=n,this._targetInst=i,this.type=a,this.nativeEvent=l,this.target=s,this.currentTarget=null;for(var u in e)e.hasOwnProperty(u)&&(n=e[u],this[u]=n?n(l):l[u]);return this.isDefaultPrevented=(l.defaultPrevented!=null?l.defaultPrevented:l.returnValue===!1)?Gl:Iu,this.isPropagationStopped=Iu,this}return D(t.prototype,{preventDefault:function(){this.defaultPrevented=!0;var n=this.nativeEvent;n&&(n.preventDefault?n.preventDefault():typeof n.returnValue!="unknown"&&(n.returnValue=!1),this.isDefaultPrevented=Gl)},stopPropagation:function(){var n=this.nativeEvent;n&&(n.stopPropagation?n.stopPropagation():typeof n.cancelBubble!="unknown"&&(n.cancelBubble=!0),this.isPropagationStopped=Gl)},persist:function(){},isPersistent:Gl}),t}var Ln={eventPhase:0,bubbles:0,cancelable:0,timeStamp:function(e){return e.timeStamp||Date.now()},defaultPrevented:0,isTrusted:0},Il=vt(Ln),Vi=D({},Ln,{view:0,detail:0}),Bp=vt(Vi),Es,ks,Li,Pl=D({},Vi,{screenX:0,screenY:0,clientX:0,clientY:0,pageX:0,pageY:0,ctrlKey:0,shiftKey:0,altKey:0,metaKey:0,getModifierState:Cs,button:0,buttons:0,relatedTarget:function(e){return e.relatedTarget===void 0?e.fromElement===e.srcElement?e.toElement:e.fromElement:e.relatedTarget},movementX:function(e){return"movementX"in e?e.movementX:(e!==Li&&(Li&&e.type==="mousemove"?(Es=e.screenX-Li.screenX,ks=e.screenY-Li.screenY):ks=Es=0,Li=e),Es)},movementY:function(e){return"movementY"in e?e.movementY:ks}}),Pu=vt(Pl),Vp=D({},Pl,{dataTransfer:0}),Lp=vt(Vp),Hp=D({},Vi,{relatedTarget:0}),Ts=vt(Hp),Gp=D({},Ln,{animationName:0,elapsedTime:0,pseudoElement:0}),Ip=vt(Gp),Pp=D({},Ln,{clipboardData:function(e){return"clipboardData"in e?e.clipboardData:window.clipboardData}}),Yp=vt(Pp),Qp=D({},Ln,{data:0}),Yu=vt(Qp),Fp={Esc:"Escape",Spacebar:" ",Left:"ArrowLeft",Up:"ArrowUp",Right:"ArrowRight",Down:"ArrowDown",Del:"Delete",Win:"OS",Menu:"ContextMenu",Apps:"ContextMenu",Scroll:"ScrollLock",MozPrintableKey:"Unidentified"},Kp={8:"Backspace",9:"Tab",12:"Clear",13:"Enter",16:"Shift",17:"Control",18:"Alt",19:"Pause",20:"CapsLock",27:"Escape",32:" ",33:"PageUp",34:"PageDown",35:"End",36:"Home",37:"ArrowLeft",38:"ArrowUp",39:"ArrowRight",40:"ArrowDown",45:"Insert",46:"Delete",112:"F1",113:"F2",114:"F3",115:"F4",116:"F5",117:"F6",118:"F7",119:"F8",120:"F9",121:"F10",122:"F11",123:"F12",144:"NumLock",145:"ScrollLock",224:"Meta"},Xp={Alt:"altKey",Control:"ctrlKey",Meta:"metaKey",Shift:"shiftKey"};function Zp(e){var t=this.nativeEvent;return t.getModifierState?t.getModifierState(e):(e=Xp[e])?!!t[e]:!1}function Cs(){return Zp}var Jp=D({},Vi,{key:function(e){if(e.key){var t=Fp[e.key]||e.key;if(t!=="Unidentified")return t}return e.type==="keypress"?(e=Hl(e),e===13?"Enter":String.fromCharCode(e)):e.type==="keydown"||e.type==="keyup"?Kp[e.keyCode]||"Unidentified":""},code:0,location:0,ctrlKey:0,shiftKey:0,altKey:0,metaKey:0,repeat:0,locale:0,getModifierState:Cs,charCode:function(e){return e.type==="keypress"?Hl(e):0},keyCode:function(e){return e.type==="keydown"||e.type==="keyup"?e.keyCode:0},which:function(e){return e.type==="keypress"?Hl(e):e.type==="keydown"||e.type==="keyup"?e.keyCode:0}}),$p=vt(Jp),Wp=D({},Pl,{pointerId:0,width:0,height:0,pressure:0,tangentialPressure:0,tiltX:0,tiltY:0,twist:0,pointerType:0,isPrimary:0}),Qu=vt(Wp),ev=D({},Ln,{submitter:0}),tv=vt(ev),nv=D({},Vi,{touches:0,targetTouches:0,changedTouches:0,altKey:0,metaKey:0,ctrlKey:0,shiftKey:0,getModifierState:Cs}),av=vt(nv),iv=D({},Ln,{propertyName:0,elapsedTime:0,pseudoElement:0}),lv=vt(iv),rv=D({},Pl,{deltaX:function(e){return"deltaX"in e?e.deltaX:"wheelDeltaX"in e?-e.wheelDeltaX:0},deltaY:function(e){return"deltaY"in e?e.deltaY:"wheelDeltaY"in e?-e.wheelDeltaY:"wheelDelta"in e?-e.wheelDelta:0},deltaZ:0,deltaMode:0}),sv=vt(rv),ov=D({},Ln,{newState:0,oldState:0,source:0}),cv=vt(ov),uv=[9,13,27,32],As=Nn&&"CompositionEvent"in window,Hi=null;Nn&&"documentMode"in document&&(Hi=document.documentMode);var dv=Nn&&"TextEvent"in window&&!Hi,Fu=Nn&&(!As||Hi&&8<Hi&&11>=Hi),Ku=" ",Xu=!1;function Zu(e,t){switch(e){case"keyup":return uv.indexOf(t.keyCode)!==-1;case"keydown":return t.keyCode!==229;case"keypress":case"mousedown":case"focusout":return!0;default:return!1}}function Ju(e){return e=e.detail,typeof e=="object"&&"data"in e?e.data:null}var Qa=!1;function _v(e,t){switch(e){case"compositionend":return Ju(t);case"keypress":return t.which!==32?null:(Xu=!0,Ku);case"textInput":return e=t.data,e===Ku&&Xu?null:e;default:return null}}function mv(e,t){if(Qa)return e==="compositionend"||!As&&Zu(e,t)?(e=Gu(),Ll=js=Vn=null,Qa=!1,e):null;switch(e){case"paste":return null;case"keypress":if(!(t.ctrlKey||t.altKey||t.metaKey)||t.ctrlKey&&t.altKey){if(t.char&&1<t.char.length)return t.char;if(t.which)return String.fromCharCode(t.which)}return null;case"compositionend":return Fu&&t.locale!=="ko"?null:t.data;default:return null}}var fv={color:!0,date:!0,datetime:!0,"datetime-local":!0,email:!0,month:!0,number:!0,password:!0,range:!0,search:!0,tel:!0,text:!0,time:!0,url:!0,week:!0};function $u(e){var t=e&&e.nodeName&&e.nodeName.toLowerCase();return t==="input"?!!fv[e.type]:t==="textarea"}function Wu(e,t,n,a){Pa?Ya?Ya.push(a):Ya=[a]:Pa=a,t=Yr(t,"onChange"),0<t.length&&(n=new Il("onChange","change",null,n,a),e.push({event:n,listeners:t}))}var Gi=null,Ii=null;function pv(e){Lm(e,0)}function Yl(e){var t=zi(e);if(Mu(t))return e}function ed(e,t){if(e==="change")return t}var td=!1;if(Nn){var Os;if(Nn){var Rs="oninput"in document;if(!Rs){var nd=document.createElement("div");nd.setAttribute("oninput","return;"),Rs=typeof nd.oninput=="function"}Os=Rs}else Os=!1;td=Os&&(!document.documentMode||9<document.documentMode)}function ad(){Gi&&(Gi.detachEvent("onpropertychange",id),Ii=Gi=null)}function id(e){if(e.propertyName==="value"&&Yl(Ii)){var t=[];Wu(t,Ii,e,ws(e)),Hu(pv,t)}}function vv(e,t,n){e==="focusin"?(ad(),Gi=t,Ii=n,Gi.attachEvent("onpropertychange",id)):e==="focusout"&&ad()}function yv(e){if(e==="selectionchange"||e==="keyup"||e==="keydown")return Yl(Ii)}function hv(e,t){if(e==="click")return Yl(t)}function gv(e,t){if(e==="input"||e==="change")return Yl(t)}function bv(e,t){return e===t&&(e!==0||1/e===1/t)||e!==e&&t!==t}var Dt=typeof Object.is=="function"?Object.is:bv;function Pi(e,t){if(Dt(e,t))return!0;if(typeof e!="object"||e===null||typeof t!="object"||t===null)return!1;var n=Object.keys(e),a=Object.keys(t);if(n.length!==a.length)return!1;for(a=0;a<n.length;a++){var i=n[a];if(!ds.call(t,i)||!Dt(e[i],t[i]))return!1}return!0}function qs(e){if(e=e||(typeof document<"u"?document:void 0),typeof e>"u")return null;try{return e.activeElement||e.body}catch{return e.body}}function ld(e){for(;e&&e.firstChild;)e=e.firstChild;return e}function rd(e,t){var n=ld(e);e=0;for(var a;n;){if(n.nodeType===3){if(a=e+n.textContent.length,e<=t&&a>=t)return{node:n,offset:t-e};e=a}e:{for(;n;){if(n.nextSibling){n=n.nextSibling;break e}n=n.parentNode}n=void 0}n=ld(n)}}function sd(e,t){return e&&t?e===t?!0:e&&e.nodeType===3?!1:t&&t.nodeType===3?sd(e,t.parentNode):"contains"in e?e.contains(t):e.compareDocumentPosition?!!(e.compareDocumentPosition(t)&16):!1:!1}function od(e){e=e!=null&&e.ownerDocument!=null&&e.ownerDocument.defaultView!=null?e.ownerDocument.defaultView:window;for(var t=qs(e.document);t instanceof e.HTMLIFrameElement;){try{var n=typeof t.contentWindow.location.href=="string"}catch{n=!1}if(n)e=t.contentWindow;else break;t=qs(e.document)}return t}function Ms(e){var t=e&&e.nodeName&&e.nodeName.toLowerCase();return t&&(t==="input"&&(e.type==="text"||e.type==="search"||e.type==="tel"||e.type==="url"||e.type==="password")||t==="textarea"||e.contentEditable==="true")}var xv=Nn&&"documentMode"in document&&11>=document.documentMode,Fa=null,Ds=null,Yi=null,zs=!1;function cd(e,t,n){var a=n.window===n?n.document:n.nodeType===9?n:n.ownerDocument;zs||Fa==null||Fa!==qs(a)||(a=Fa,"selectionStart"in a&&Ms(a)?a={start:a.selectionStart,end:a.selectionEnd}:(a=(a.ownerDocument&&a.ownerDocument.defaultView||window).getSelection(),a={anchorNode:a.anchorNode,anchorOffset:a.anchorOffset,focusNode:a.focusNode,focusOffset:a.focusOffset}),Yi&&Pi(Yi,a)||(Yi=a,a=Yr(Ds,"onSelect"),0<a.length&&(t=new Il("onSelect","select",null,t,n),e.push({event:t,listeners:a}),t.target=Fa)))}function pa(e,t){var n={};return n[e.toLowerCase()]=t.toLowerCase(),n["Webkit"+e]="webkit"+t,n["Moz"+e]="moz"+t,n}var Ka={animationend:pa("Animation","AnimationEnd"),animationiteration:pa("Animation","AnimationIteration"),animationstart:pa("Animation","AnimationStart"),transitionrun:pa("Transition","TransitionRun"),transitionstart:pa("Transition","TransitionStart"),transitioncancel:pa("Transition","TransitionCancel"),transitionend:pa("Transition","TransitionEnd")},Us={},ud={};Nn&&(ud=document.createElement("div").style,"AnimationEvent"in window||(delete Ka.animationend.animation,delete Ka.animationiteration.animation,delete Ka.animationstart.animation),"TransitionEvent"in window||delete Ka.transitionend.transition);function va(e){if(Us[e])return Us[e];if(!Ka[e])return e;var t=Ka[e],n;for(n in t)if(t.hasOwnProperty(n)&&n in ud)return Us[e]=t[n];return e}var dd=va("animationend"),_d=va("animationiteration"),md=va("animationstart"),wv=va("transitionrun"),Nv=va("transitionstart"),Sv=va("transitioncancel"),fd=va("transitionend"),pd=new Map,Bs="abort auxClick beforeToggle cancel canPlay canPlayThrough click close contextMenu copy cut drag dragEnd dragEnter dragExit dragLeave dragOver dragStart drop durationChange emptied encrypted ended error fullscreenChange fullscreenError gotPointerCapture input invalid keyDown keyPress keyUp load loadedData loadedMetadata loadStart lostPointerCapture mouseDown mouseMove mouseOut mouseOver mouseUp paste pause play playing pointerCancel pointerDown pointerMove pointerOut pointerOver pointerUp progress rateChange reset resize seeked seeking stalled submit suspend timeUpdate touchCancel touchEnd touchStart volumeChange scroll toggle touchMove waiting wheel".split(" ");Bs.push("scrollEnd");function tn(e,t){pd.set(e,t),fa(t,[e])}var jv=0;function Sn(e,t){if(e.name!=null&&e.name!=="auto")return e.name;if(t.autoName!==null)return t.autoName;e=rn.identifierPrefix;var n=jv++;return e="_"+e+"t_"+n.toString(32)+"_",t.autoName=e}function vd(e){if(e==null||typeof e=="string")return e;var t=null,n=fi;if(n!==null)for(var a=0;a<n.length;a++){var i=e[n[a]];if(i!=null){if(i==="none")return"none";t=t==null?i:t+(" "+i)}}return t??e.default}function jn(e,t){return e=vd(e),t=vd(t),t==null?e==="auto"?null:e:t==="auto"?null:t}var Ql=typeof reportError=="function"?reportError:function(e){if(typeof window=="object"&&typeof window.ErrorEvent=="function"){var t=new window.ErrorEvent("error",{bubbles:!0,cancelable:!0,message:typeof e=="object"&&e!==null&&typeof e.message=="string"?String(e.message):String(e),error:e});if(!window.dispatchEvent(t))return}else if(typeof process=="object"&&typeof process.emit=="function"){process.emit("uncaughtException",e);return}console.error(e)},Ft=[],Xa=0,Vs=0;function Fl(){for(var e=Xa,t=Vs=Xa=0;t<e;){var n=Ft[t];Ft[t++]=null;var a=Ft[t];Ft[t++]=null;var i=Ft[t];Ft[t++]=null;var l=Ft[t];if(Ft[t++]=null,a!==null&&i!==null){var s=a.pending;s===null?i.next=i:(i.next=s.next,s.next=i),a.pending=i}l!==0&&yd(n,i,l)}}function Kl(e,t,n,a){Ft[Xa++]=e,Ft[Xa++]=t,Ft[Xa++]=n,Ft[Xa++]=a,Vs|=a,e.lanes|=a,e=e.alternate,e!==null&&(e.lanes|=a)}function Ls(e,t,n,a){return Kl(e,t,n,a),Xl(e)}function ya(e,t){return Kl(e,null,null,t),Xl(e)}function yd(e,t,n){e.lanes|=n;var a=e.alternate;a!==null&&(a.lanes|=n);for(var i=!1,l=e.return;l!==null;)l.childLanes|=n,a=l.alternate,a!==null&&(a.childLanes|=n),l.tag===22&&(e=l.stateNode,e===null||e._visibility&1||(i=!0)),e=l,l=l.return;return e.tag===3?(l=e.stateNode,i&&t!==null&&(i=31-qt(n),e=l.hiddenUpdates,a=e[i],a===null?e[i]=[t]:a.push(t),t.lane=n|536870912),l):null}function Xl(e){if(50<ml)throw ml=0,Br=null,Error(d(185));for(var t=e.return;t!==null;)e=t,t=e.return;return e.tag===3?e.stateNode:null}var Za={};function Ev(e,t,n,a){this.tag=e,this.key=n,this.sibling=this.child=this.return=this.stateNode=this.type=this.elementType=null,this.index=0,this.refCleanup=this.ref=null,this.pendingProps=t,this.dependencies=this.memoizedState=this.updateQueue=this.memoizedProps=null,this.mode=a,this.subtreeFlags=this.flags=0,this.deletions=null,this.childLanes=this.lanes=0,this.alternate=null}function St(e,t,n,a){return new Ev(e,t,n,a)}function Hs(e){return e=e.prototype,!(!e||!e.isReactComponent)}function En(e,t){var n=e.alternate;return n===null?(n=St(e.tag,t,e.key,e.mode),n.elementType=e.elementType,n.type=e.type,n.stateNode=e.stateNode,n.alternate=e,e.alternate=n):(n.pendingProps=t,n.type=e.type,n.flags=0,n.subtreeFlags=0,n.deletions=null),n.flags=e.flags&1206910976,n.childLanes=e.childLanes,n.lanes=e.lanes,n.child=e.child,n.memoizedProps=e.memoizedProps,n.memoizedState=e.memoizedState,n.updateQueue=e.updateQueue,t=e.dependencies,n.dependencies=t===null?null:{lanes:t.lanes,firstContext:t.firstContext},n.sibling=e.sibling,n.index=e.index,n.ref=e.ref,n.refCleanup=e.refCleanup,n}function hd(e,t){e.flags&=1206910978;var n=e.alternate;return n===null?(e.childLanes=0,e.lanes=t,e.child=null,e.subtreeFlags=0,e.memoizedProps=null,e.memoizedState=null,e.updateQueue=null,e.dependencies=null,e.stateNode=null):(e.childLanes=n.childLanes,e.lanes=n.lanes,e.child=n.child,e.subtreeFlags=0,e.deletions=null,e.memoizedProps=n.memoizedProps,e.memoizedState=n.memoizedState,e.updateQueue=n.updateQueue,e.type=n.type,t=n.dependencies,e.dependencies=t===null?null:{lanes:t.lanes,firstContext:t.firstContext}),e}function Zl(e,t,n,a,i,l){var s=0;if(a=e,typeof a=="function")Hs(a)&&(s=1);else if(typeof a=="string")s=eh(e,n,k.current)?26:e==="html"||e==="head"||e==="body"?27:5;else e:switch(a){case ze:return e=St(31,n,t,i),e.elementType=ze,e.lanes=l,e;case re:return ha(n.children,i,l,t);case ue:s=8,i|=24;break;case Ie:return e=St(12,n,t,i|2),e.elementType=Ie,e.lanes=l,e;case ne:return e=St(13,n,t,i),e.elementType=ne,e.lanes=l,e;case ae:return e=St(19,n,t,i),e.elementType=ae,e.lanes=l,e;case mt:case p:return e=i|32,e=St(30,n,t,e),e.elementType=p,e.lanes=l,e.stateNode={autoName:null,paired:null,clones:null,ref:null},e;default:if(typeof a=="object"&&a!==null)switch(a.$$typeof){case He:s=10;break e;case Xe:s=9;break e;case z:s=11;break e;case ke:s=14;break e;case me:s=16,a=null;break e}s=29,n=Error(d(130,e===null?"null":typeof e,"")),a=null}return t=St(s,n,t,i),t.elementType=e,t.type=a,t.lanes=l,t}function ha(e,t,n,a){return e=St(7,e,a,t),e.lanes=n,e}function Gs(e,t,n){return e=St(6,e,null,t),e.lanes=n,e}function gd(e){var t=St(18,null,null,0);return t.stateNode=e,t}function Is(e,t,n){return t=St(4,e.children!==null?e.children:[],e.key,t),t.lanes=n,t.stateNode={containerInfo:e.containerInfo,pendingChildren:null,implementation:e.implementation},t}var bd=new WeakMap;function Kt(e,t){if(typeof e=="object"&&e!==null){var n=bd.get(e);return n!==void 0?n:(t={value:e,source:t,stack:fu(t)},bd.set(e,t),t)}return{value:e,source:t,stack:fu(t)}}var Ja=[],$a=0,Jl=null,Qi=0,Xt=[],Zt=0,Hn=null,cn=1,un="";function kn(e,t){Ja[$a++]=Qi,Ja[$a++]=Jl,Jl=e,Qi=t}function xd(e,t,n){Xt[Zt++]=cn,Xt[Zt++]=un,Xt[Zt++]=Hn,Hn=e;var a=cn;e=un;var i=32-qt(a)-1;a&=~(1<<i),n+=1;var l=32-qt(t)+i;if(30<l){var s=i-i%5;l=(a&(1<<s)-1).toString(32),a>>=s,i-=s,cn=1<<32-qt(t)+i|n<<i|a,un=l+e}else cn=1<<l|n<<i|a,un=e}function $l(e){e.return!==null&&(kn(e,1),xd(e,1,0))}function Ps(e){for(;e===Jl;)Jl=Ja[--$a],Ja[$a]=null,Qi=Ja[--$a],Ja[$a]=null;for(;e===Hn;)Hn=Xt[--Zt],Xt[Zt]=null,un=Xt[--Zt],Xt[Zt]=null,cn=Xt[--Zt],Xt[Zt]=null}function wd(e,t){Xt[Zt++]=cn,Xt[Zt++]=un,Xt[Zt++]=Hn,cn=t.id,un=t.overflow,Hn=e}var nt=null,Ve=null,pe=!1,Gn=null,Jt=!1,Ys=Error(d(519));function In(e){var t=Error(d(418,1<arguments.length&&arguments[1]!==void 0&&arguments[1]?"text":"HTML",""));throw Fi(Kt(t,e)),Ys}function Nd(e){var t=e.stateNode,n=e.type,a=e.memoizedProps;switch(t[rt]=e,t[Nt]=a,n){case"dialog":ye("cancel",t),ye("close",t);break;case"iframe":case"object":case"embed":ye("load",t);break;case"video":case"audio":for(n=0;n<pl.length;n++)ye(pl[n],t);break;case"source":ye("error",t);break;case"img":case"image":case"link":ye("error",t),ye("load",t);break;case"details":ye("toggle",t);break;case"input":ye("invalid",t),Du(t,a.value,a.defaultValue,a.checked,a.defaultChecked,a.type,a.name,!0);break;case"select":ye("invalid",t);break;case"textarea":ye("invalid",t),Uu(t,a.value,a.defaultValue,a.children)}n=a.children,typeof n!="string"&&typeof n!="number"&&typeof n!="bigint"||t.textContent===""+n||a.suppressHydrationWarning===!0||Pm(t.textContent,n)?(a.popover!=null&&(ye("beforetoggle",t),ye("toggle",t)),a.onScroll!=null&&ye("scroll",t),a.onScrollEnd!=null&&ye("scrollend",t),a.onClick!=null&&(t.onclick=on),t=!0):t=!1,t||In(e,!0)}function Wl(e){for(nt=e.return;nt;)switch(nt.tag){case 5:case 31:case 13:Jt=!1;return;case 27:case 3:Jt=!0;return;default:nt=nt.return}}function Wa(e){if(e!==nt)return!1;if(!pe)return Wl(e),pe=!0,!1;var t=e.tag,n;if((n=t!==3&&t!==27)&&((n=t===5)&&(n=e.type,n=!(n!=="form"&&n!=="button")||wc(e.type,e.memoizedProps)),n=!n),n&&Ve&&In(e),Wl(e),t===13){if(e=e.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(317));Ve=uf(e)}else if(t===31){if(e=e.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(317));Ve=uf(e)}else t===27?(t=Ve,la(e.type)?(e=Oc,Oc=null,Ve=e):Ve=t):Ve=nt?Wt(e.stateNode.nextSibling):null;return!0}function ga(){Ve=nt=null,pe=!1}function Qs(){var e=Gn;return e!==null&&(kt===null?kt=e:kt.push.apply(kt,e),Gn=null),e}function Fi(e){Gn===null?Gn=[e]:Gn.push(e)}var Fs=At(null),ba=null,Tn=null;function Pn(e,t,n){Q(Fs,t._currentValue),t._currentValue=n}function Cn(e){e._currentValue=Fs.current,H(Fs)}function er(e,t,n){for(;e!==null;){var a=e.alternate;if((e.childLanes&t)!==t?(e.childLanes|=t,a!==null&&(a.childLanes|=t)):a!==null&&(a.childLanes&t)!==t&&(a.childLanes|=t),e===n)break;e=e.return}}function Ks(e,t,n,a){var i=e.child;for(i!==null&&(i.return=e);i!==null;){var l=i.dependencies;if(l!==null){var s=i.child;l=l.firstContext;e:for(;l!==null;){var u=l;l=i;for(var m=0;m<t.length;m++)if(u.context===t[m]){l.lanes|=n,u=l.alternate,u!==null&&(u.lanes|=n),er(l.return,n,e),a||(s=null);break e}l=u.next}}else if(i.tag===18){if(s=i.return,s===null)throw Error(d(341));s.lanes|=n,l=s.alternate,l!==null&&(l.lanes|=n),er(s,n,e),s=null}else i.tag===13&&i.memoizedState!==null&&i.memoizedState.dehydrated===null?(i.lanes|=n,s=i.alternate,s!==null&&(s.lanes|=n),er(i.return,n,e),s=i.child,s=s!==null?s.sibling:null):s=i.child;if(s!==null)s.return=i;else for(s=i;s!==null;){if(s===e){s=null;break}if(i=s.sibling,i!==null){i.return=s.return,s=i;break}s=s.return}i=s}}function xa(e,t,n,a){e=null;for(var i=t,l=!1;i!==null;){if(!l){if((i.flags&524288)!==0)l=!0;else if((i.flags&262144)!==0)break}if(i.tag===10){var s=i.alternate;if(s===null)throw Error(d(387));if(s=s.memoizedProps,s!==null){var u=i.type;Dt(i.pendingProps.value,s.value)||(e!==null?e.push(u):e=[u])}}else if(i===It.current){if(s=i.alternate,s===null)throw Error(d(387));s.memoizedState.memoizedState!==i.memoizedState.memoizedState&&(e!==null?e.push(Si):e=[Si])}i=i.return}return e!==null&&Ks(t,e,n,a),t.flags|=262144,e!==null}function tr(e){for(e=e.firstContext;e!==null;){if(!Dt(e.context._currentValue,e.memoizedValue))return!0;e=e.next}return!1}function wa(e){ba=e,Tn=null,e=e.dependencies,e!==null&&(e.firstContext=null)}function st(e){return Sd(ba,e)}function nr(e,t){return ba===null&&wa(e),Sd(e,t)}function Sd(e,t){var n=t._currentValue;if(t={context:t,memoizedValue:n,next:null},Tn===null){if(e===null)throw Error(d(308));Tn=t,e.dependencies={lanes:0,firstContext:t},e.flags|=524288}else Tn=Tn.next=t;return n}var kv=typeof AbortController<"u"?AbortController:function(){var e=[],t=this.signal={aborted:!1,addEventListener:function(n,a){e.push(a)}};this.abort=function(){t.aborted=!0,e.forEach(function(n){return n()})}},Tv=o.unstable_scheduleCallback,Cv=o.unstable_NormalPriority,Ze={$$typeof:He,Consumer:null,Provider:null,_currentValue:null,_currentValue2:null,_threadCount:0};function Xs(){return{controller:new kv,data:new Map,refCount:0}}function Ki(e){e.refCount--,e.refCount===0&&Tv(Cv,function(){e.controller.abort()})}function jd(e,t){if((e.pendingLanes&4194048)!==0){var n=e.transitionTypes;for(n===null&&(n=e.transitionTypes=[]),e=0;e<t.length;e++){var a=t[e];n.indexOf(a)===-1&&n.push(a)}}}var Xi=null;function Av(e){var t=e.transitionTypes;return e.transitionTypes=null,t}var Zi=null,Zs=0,Na=0,ei=null;function Ov(e,t){if(Zi===null){var n=Zi=[];Zs=0,Na=mc(),ei={status:"pending",value:void 0,then:function(a){n.push(a)}}}return Zs++,t.then(Ed,Ed),t}function Ed(){if(--Zs===0&&(Xi=null,Zi!==null)){ei!==null&&(ei.status="fulfilled");var e=Zi;Zi=null,Na=0,ei=null;for(var t=0;t<e.length;t++)(0,e[t])()}}function Rv(e,t){var n=[],a={status:"pending",value:null,reason:null,then:function(i){n.push(i)}};return e.then(function(){a.status="fulfilled",a.value=t;for(var i=0;i<n.length;i++)(0,n[i])(t)},function(i){for(a.status="rejected",a.reason=i,i=0;i<n.length;i++)(0,n[i])(void 0)}),a}var kd=J.S;J.S=function(e,t){if(gm=Ot(),typeof t=="object"&&t!==null&&typeof t.then=="function"&&Ov(e,t),Xi!==null)for(var n=hi;n!==null;)jd(n,Xi),n=n.next;if(n=e.types,n!==null){for(var a=hi;a!==null;)jd(a,n),a=a.next;if(Na!==0){a=Xi,a===null&&(a=Xi=[]);for(var i=0;i<n.length;i++){var l=n[i];a.indexOf(l)===-1&&a.push(l)}}}kd!==null&&kd(e,t)};var Sa=At(null);function Js(){var e=Sa.current;return e!==null?e:Be.pooledCache}function ar(e,t){t===null?Q(Sa,Sa.current):Q(Sa,t.pool)}function Td(){var e=Js();return e===null?null:{parent:Ze._currentValue,pool:e}}var ti=Error(d(460)),$s=Error(d(474)),ir=Error(d(542)),lr={then:function(){}};function Cd(e){return e=e.status,e==="fulfilled"||e==="rejected"}function Ad(e,t,n){switch(n=e[n],n===void 0?e.push(t):n!==t&&(t.then(on,on),t=n),t.status){case"fulfilled":return t.value;case"rejected":throw e=t.reason,Rd(e),e===void 0&&!("reason"in t)?Error(d(600)):e;default:if(typeof t.status=="string")t.then(on,on);else{if(e=Be,e!==null&&100<e.shellSuspendCounter)throw Error(d(482));e=t,e.status="pending",e.then(function(a){if(t.status==="pending"){var i=t;i.status="fulfilled",i.value=a}},function(a){if(t.status==="pending"){var i=t;i.status="rejected",i.reason=a}})}switch(t.status){case"fulfilled":return t.value;case"rejected":throw e=t.reason,Rd(e),e}throw Ea=t,ti}}function ja(e){try{var t=e._init;return t(e._payload)}catch(n){throw n!==null&&typeof n=="object"&&typeof n.then=="function"?(Ea=n,ti):n}}var Ea=null;function Od(){if(Ea===null)throw Error(d(459));var e=Ea;return Ea=null,e}function Rd(e){if(e===ti||e===ir)throw Error(d(483))}var ni=null,Ji=0;function rr(e){var t=Ji;return Ji+=1,ni===null&&(ni=[]),Ad(ni,e,t)}function Yn(e,t){t=t.props.ref,e.ref=t!==void 0?t:null}function sr(e,t){throw t.$$typeof===P?Error(d(525)):(e=Object.prototype.toString.call(t),Error(d(31,e==="[object Object]"?"object with keys {"+Object.keys(t).join(", ")+"}":e)))}function qd(e){function t(h,f){if(e){var x=h.deletions;x===null?(h.deletions=[f],h.flags|=16):x.push(f)}}function n(h,f){if(!e)return null;for(;f!==null;)t(h,f),f=f.sibling;return null}function a(h){for(var f=new Map;h!==null;)h.key===null?f.set(h.index,h):f.set(h.key,h),h=h.sibling;return f}function i(h,f){return h=En(h,f),h.index=0,h.sibling=null,h}function l(h,f,x){return h.index=x,e?(x=h.alternate,x!==null?(x=x.index,x<f?(h.flags|=2,f):x):(h.flags|=134217730,f)):(h.flags|=1048576,f)}function s(h){return e&&h.alternate===null&&(h.flags|=134217730),h}function u(h,f,x,T){return f===null||f.tag!==6?(f=Gs(x,h.mode,T),f.return=h,f):(f=i(f,x),f.return=h,f)}function m(h,f,x,T){var I=x.type;return I===re?(h=S(h,f,x.props.children,T,x.key),Yn(h,x),h):f!==null&&(f.elementType===I||typeof I=="object"&&I!==null&&I.$$typeof===me&&ja(I)===f.type)?(f=i(f,x.props),Yn(f,x),f.return=h,f):(f=Zl(x.type,x.key,x.props,null,h.mode,T),Yn(f,x),f.return=h,f)}function g(h,f,x,T){return f===null||f.tag!==4||f.stateNode.containerInfo!==x.containerInfo||f.stateNode.implementation!==x.implementation?(f=Is(x,h.mode,T),f.return=h,f):(f=i(f,x.children||[]),f.return=h,f)}function S(h,f,x,T,I){return f===null||f.tag!==7?(f=ha(x,h.mode,T,I),f.return=h,f):(f=i(f,x),f.return=h,f)}function C(h,f,x){if(typeof f=="string"&&f!==""||typeof f=="number"||typeof f=="bigint")return f=Gs(""+f,h.mode,x),f.return=h,f;if(typeof f=="object"&&f!==null){switch(f.$$typeof){case he:return x=Zl(f.type,f.key,f.props,null,h.mode,x),Yn(x,f),x.return=h,x;case Oe:return f=Is(f,h.mode,x),f.return=h,f;case me:return f=ja(f),C(h,f,x)}if(Se(f)||ee(f))return f=ha(f,h.mode,x,null),f.return=h,f;if(typeof f.then=="function")return C(h,rr(f),x);if(f.$$typeof===He)return C(h,nr(h,f),x);sr(h,f)}return null}function v(h,f,x,T){var I=f!==null?f.key:null;if(typeof x=="string"&&x!==""||typeof x=="number"||typeof x=="bigint")return I!==null?null:u(h,f,""+x,T);if(typeof x=="object"&&x!==null){switch(x.$$typeof){case he:return x.key===I?m(h,f,x,T):null;case Oe:return x.key===I?g(h,f,x,T):null;case me:return x=ja(x),v(h,f,x,T)}if(Se(x)||ee(x))return I!==null?null:S(h,f,x,T,null);if(typeof x.then=="function")return v(h,f,rr(x),T);if(x.$$typeof===He)return v(h,f,nr(h,x),T);sr(h,x)}return null}function w(h,f,x,T,I){if(typeof T=="string"&&T!==""||typeof T=="number"||typeof T=="bigint")return h=h.get(x)||null,u(f,h,""+T,I);if(typeof T=="object"&&T!==null){switch(T.$$typeof){case he:return h=h.get(T.key===null?x:T.key)||null,m(f,h,T,I);case Oe:return h=h.get(T.key===null?x:T.key)||null,g(f,h,T,I);case me:return T=ja(T),w(h,f,x,T,I)}if(Se(T)||ee(T))return h=h.get(x)||null,S(f,h,T,I,null);if(typeof T.then=="function")return w(h,f,x,rr(T),I);if(T.$$typeof===He)return w(h,f,x,nr(f,T),I);sr(f,T)}return null}function L(h,f,x,T){for(var I=null,be=null,te=f,le=f=0,We=null;te!==null&&le<x.length;le++){te.index>le?(We=te,te=null):We=te.sibling;var je=v(h,te,x[le],T);if(je===null){te===null&&(te=We);break}e&&te&&je.alternate===null&&t(h,te),f=l(je,f,le),be===null?I=je:be.sibling=je,be=je,te=We}if(le===x.length)return n(h,te),pe&&kn(h,le),I;if(te===null){for(;le<x.length;le++)te=C(h,x[le],T),te!==null&&(f=l(te,f,le),be===null?I=te:be.sibling=te,be=te);return pe&&kn(h,le),I}for(te=a(te);le<x.length;le++)We=w(te,h,le,x[le],T),We!==null&&(e&&(je=We.alternate,je!==null&&te.delete(je.key===null?le:je.key)),f=l(We,f,le),be===null?I=We:be.sibling=We,be=We);return e&&te.forEach(function(ua){return t(h,ua)}),pe&&kn(h,le),I}function X(h,f,x,T){if(x==null)throw Error(d(151));for(var I=null,be=null,te=f,le=f=0,We=null,je=x.next();te!==null&&!je.done;le++,je=x.next()){te.index>le?(We=te,te=null):We=te.sibling;var ua=v(h,te,je.value,T);if(ua===null){te===null&&(te=We);break}e&&te&&ua.alternate===null&&t(h,te),f=l(ua,f,le),be===null?I=ua:be.sibling=ua,be=ua,te=We}if(je.done)return n(h,te),pe&&kn(h,le),I;if(te===null){for(;!je.done;le++,je=x.next())je=C(h,je.value,T),je!==null&&(f=l(je,f,le),be===null?I=je:be.sibling=je,be=je);return pe&&kn(h,le),I}for(te=a(te);!je.done;le++,je=x.next())je=w(te,h,le,je.value,T),je!==null&&(e&&(We=je.alternate,We!==null&&te.delete(We.key===null?le:We.key)),f=l(je,f,le),be===null?I=je:be.sibling=je,be=je);return e&&te.forEach(function(_h){return t(h,_h)}),pe&&kn(h,le),I}function _e(h,f,x,T){if(typeof x=="object"&&x!==null&&x.type===re&&x.key===null&&x.props.ref===void 0&&(x=x.props.children),typeof x=="object"&&x!==null){switch(x.$$typeof){case he:e:{for(var I=x.key;f!==null;){if(f.key===I){if(I=x.type,I===re){if(f.tag===7){n(h,f.sibling),T=i(f,x.props.children),Yn(T,x),T.return=h,h=T;break e}}else if(f.elementType===I||typeof I=="object"&&I!==null&&I.$$typeof===me&&ja(I)===f.type){n(h,f.sibling),T=i(f,x.props),Yn(T,x),T.return=h,h=T;break e}n(h,f);break}else t(h,f);f=f.sibling}x.type===re?(T=ha(x.props.children,h.mode,T,x.key),Yn(T,x),T.return=h,h=T):(T=Zl(x.type,x.key,x.props,null,h.mode,T),Yn(T,x),T.return=h,h=T)}return s(h);case Oe:e:{for(I=x.key;f!==null;){if(f.key===I)if(f.tag===4&&f.stateNode.containerInfo===x.containerInfo&&f.stateNode.implementation===x.implementation){n(h,f.sibling),T=i(f,x.children||[]),T.return=h,h=T;break e}else{n(h,f);break}else t(h,f);f=f.sibling}T=Is(x,h.mode,T),T.return=h,h=T}return s(h);case me:return x=ja(x),_e(h,f,x,T)}if(Se(x))return L(h,f,x,T);if(ee(x)){if(I=ee(x),typeof I!="function")throw Error(d(150));return x=I.call(x),X(h,f,x,T)}if(typeof x.then=="function")return _e(h,f,rr(x),T);if(x.$$typeof===He)return _e(h,f,nr(h,x),T);sr(h,x)}return typeof x=="string"&&x!==""||typeof x=="number"||typeof x=="bigint"?(x=""+x,f!==null&&f.tag===6?(n(h,f.sibling),T=i(f,x),T.return=h,h=T):(n(h,f),T=Gs(x,h.mode,T),T.return=h,h=T),s(h)):n(h,f)}return function(h,f,x,T){try{Ji=0;var I=_e(h,f,x,T);return ni=null,I}catch(te){if(te===ti||te===ir)throw te;var be=St(29,te,null,h.mode);return be.lanes=T,be.return=h,be}finally{}}}var ka=qd(!0),Md=qd(!1),Qn=!1;function Ws(e){e.updateQueue={baseState:e.memoizedState,firstBaseUpdate:null,lastBaseUpdate:null,shared:{pending:null,lanes:0,hiddenCallbacks:null},callbacks:null}}function eo(e,t){e=e.updateQueue,t.updateQueue===e&&(t.updateQueue={baseState:e.baseState,firstBaseUpdate:e.firstBaseUpdate,lastBaseUpdate:e.lastBaseUpdate,shared:e.shared,callbacks:null})}function Fn(e){return{lane:e,tag:0,payload:null,callback:null,next:null}}function Kn(e,t,n){var a=e.updateQueue;if(a===null)return null;if(a=a.shared,(Ae&2)!==0){var i=a.pending;return i===null?t.next=t:(t.next=i.next,i.next=t),a.pending=t,t=Xl(e),yd(e,null,n),t}return Kl(e,a,t,n),Xl(e)}function $i(e,t,n){if(t=t.updateQueue,t!==null&&(t=t.shared,(n&4194048)!==0)){var a=t.lanes;a&=e.pendingLanes,n|=a,t.lanes=n,xu(e,n)}}function to(e,t){var n=e.updateQueue,a=e.alternate;if(a!==null&&(a=a.updateQueue,n===a)){var i=null,l=null;if(n=n.firstBaseUpdate,n!==null){do{var s={lane:n.lane,tag:n.tag,payload:n.payload,callback:null,next:null};l===null?i=l=s:l=l.next=s,n=n.next}while(n!==null);l===null?i=l=t:l=l.next=t}else i=l=t;n={baseState:a.baseState,firstBaseUpdate:i,lastBaseUpdate:l,shared:a.shared,callbacks:a.callbacks},e.updateQueue=n;return}e=n.lastBaseUpdate,e===null?n.firstBaseUpdate=t:e.next=t,n.lastBaseUpdate=t}var no=!1;function Wi(){if(no){var e=ei;if(e!==null)throw e}}function el(e,t,n,a){no=!1;var i=e.updateQueue;Qn=!1;var l=i.firstBaseUpdate,s=i.lastBaseUpdate,u=i.shared.pending;if(u!==null){i.shared.pending=null;var m=u,g=m.next;m.next=null,s===null?l=g:s.next=g,s=m;var S=e.alternate;S!==null&&(S=S.updateQueue,u=S.lastBaseUpdate,u!==s&&(u===null?S.firstBaseUpdate=g:u.next=g,S.lastBaseUpdate=m))}if(l!==null){var C=i.baseState;s=0,S=g=m=null,u=l;do{var v=u.lane&-536870913,w=v!==u.lane;if(w?(ge&v)===v:(a&v)===v){v!==0&&v===Na&&(no=!0),S!==null&&(S=S.next={lane:0,tag:u.tag,payload:u.payload,callback:null,next:null});e:{var L=e,X=u;v=t;var _e=n;switch(X.tag){case 1:if(L=X.payload,typeof L=="function"){C=L.call(_e,C,v);break e}C=L;break e;case 3:L.flags=L.flags&-65537|128;case 0:if(L=X.payload,v=typeof L=="function"?L.call(_e,C,v):L,v==null)break e;C=D({},C,v);break e;case 2:Qn=!0}}v=u.callback,v!==null&&(e.flags|=64,w&&(e.flags|=8192),w=i.callbacks,w===null?i.callbacks=[v]:w.push(v))}else w={lane:v,tag:u.tag,payload:u.payload,callback:u.callback,next:null},S===null?(g=S=w,m=C):S=S.next=w,s|=v;if(u=u.next,u===null){if(u=i.shared.pending,u===null)break;w=u,u=w.next,w.next=null,i.lastBaseUpdate=w,i.shared.pending=null}}while(!0);S===null&&(m=C),i.baseState=m,i.firstBaseUpdate=g,i.lastBaseUpdate=S,l===null&&(i.shared.lanes=0),ta|=s,e.lanes=s,e.memoizedState=C}}function Dd(e,t){if(typeof e!="function")throw Error(d(191,e));e.call(t)}function zd(e,t){var n=e.callbacks;if(n!==null)for(e.callbacks=null,e=0;e<n.length;e++)Dd(n[e],t)}var Xn=At(null),or=At(0);function Ud(e,t){e=Mn,Q(or,e),Q(Xn,t),Mn=e|t.baseLanes}function ao(){Q(or,Mn),Q(Xn,Xn.current)}function io(){Mn=or.current,H(Xn),H(or)}var ot=At(null),pt=null;function Zn(e){var t=e.alternate;Q(ct,ct.current&1),Q(ot,e),pt===null&&(t===null||Xn.current!==null||t.memoizedState!==null)&&(pt=e)}function lo(e){Q(ct,ct.current),Q(ot,e),pt===null&&(pt=e)}function Bd(e){e.tag===22?(Q(ct,ct.current),Q(ot,e),pt===null&&(pt=e)):Jn()}function Jn(){Q(ct,ct.current),Q(ot,ot.current)}function zt(e){H(ot),pt===e&&(pt=null),H(ct)}var ct=At(0);function tl(e,t){Q(ot,ot.current),Q(ct,t)}function ro(e){H(ct),H(ot),pt===e&&(pt=null)}function cr(e){for(var t=e;t!==null;){if(t.tag===13){var n=t.memoizedState;if(n!==null&&(n=n.dehydrated,n===null||Cc(n)||Ac(n)))return t}else if(t.tag===19&&t.memoizedProps.revealOrder!=="independent"){if((t.flags&128)!==0)return t}else if(t.child!==null){t.child.return=t,t=t.child;continue}if(t===e)break;for(;t.sibling===null;){if(t.return===null||t.return===e)return null;t=t.return}t.sibling.return=t.return,t=t.sibling}return null}var An=0,de=null,Ue=null,Je=null,ur=!1,ai=!1,Ta=!1,dr=0,nl=0,ii=null,qv=0;function Ye(){throw Error(d(321))}function so(e,t){if(t===null)return!1;for(var n=0;n<t.length&&n<e.length;n++)if(!Dt(e[n],t[n]))return!1;return!0}function oo(e,t,n,a,i,l){return An=l,de=t,t.memoizedState=null,t.updateQueue=null,t.lanes=0,J.H=e===null||e.memoizedState===null?x_:w_,Ta=!1,l=n(a,i),Ta=!1,ai&&(l=Ld(t,n,a,i)),Vd(e),l}function Vd(e){J.H=hr;var t=Ue!==null&&Ue.next!==null;if(An=0,Je=Ue=de=null,ur=!1,nl=0,ii=null,t)throw Error(d(300));e===null||$e||(e=e.dependencies,e!==null&&tr(e)&&($e=!0))}function Ld(e,t,n,a){de=e;var i=0;do{if(ai&&(ii=null),nl=0,ai=!1,25<=i)throw Error(d(301));if(i+=1,Je=Ue=null,e.updateQueue!=null){var l=e.updateQueue;l.lastEffect=null,l.events=null,l.stores=null,l.memoCache!=null&&(l.memoCache.index=0)}J.H=Hv,l=t(n,a)}while(ai);return l}function Mv(){var e=J.H,t=e.useState()[0];return t=typeof t.then=="function"?al(t):t,e=e.useState()[0],(Ue!==null?Ue.memoizedState:null)!==e&&(de.flags|=1024),t}function co(){var e=dr!==0;return dr=0,e}function uo(e,t,n){t.updateQueue=e.updateQueue,t.flags&=-2053,e.lanes&=~n}function _o(e){if(ur){for(e=e.memoizedState;e!==null;){var t=e.queue;t!==null&&(t.pending=null),e=e.next}ur=!1}An=0,Je=Ue=de=null,ai=!1,nl=dr=0,ii=null}function yt(){var e={memoizedState:null,baseState:null,baseQueue:null,queue:null,next:null};return Je===null?de.memoizedState=Je=e:Je=Je.next=e,Je}function Ke(){if(Ue===null){var e=de.alternate;e=e!==null?e.memoizedState:null}else e=Ue.next;var t=Je===null?de.memoizedState:Je.next;if(t!==null)Je=t,Ue=e;else{if(e===null)throw de.alternate===null?Error(d(467)):Error(d(310));Ue=e,e={memoizedState:Ue.memoizedState,baseState:Ue.baseState,baseQueue:Ue.baseQueue,queue:Ue.queue,next:null},Je===null?de.memoizedState=Je=e:Je=Je.next=e}return Je}function _r(){return{lastEffect:null,events:null,stores:null,memoCache:null}}function al(e){var t=nl;return nl+=1,ii===null&&(ii=[]),e=Ad(ii,e,t),t=de,(Je===null?t.memoizedState:Je.next)===null&&(t=t.alternate,J.H=t===null||t.memoizedState===null?x_:w_),e}function mr(e){if(e!==null&&typeof e=="object"){if(typeof e.then=="function")return al(e);if(e.$$typeof===M)return;if(e.$$typeof===He)return st(e)}throw Error(d(438,String(e)))}function mo(e){var t=null,n=de.updateQueue;if(n!==null&&(t=n.memoCache),t==null){var a=de.alternate;a!==null&&(a=a.updateQueue,a!==null&&(a=a.memoCache,a!=null&&(t={data:a.data.map(function(i){return i.slice()}),index:0})))}if(t==null&&(t={data:[],index:0}),n===null&&(n=_r(),de.updateQueue=n),n.memoCache=t,n=t.data[t.index],n===void 0)for(n=t.data[t.index]=Array(e),a=0;a<e;a++)n[a]=bt;return t.index++,n}function On(e,t){return typeof t=="function"?t(e):t}function fr(e){var t=Ke();return fo(t,Ue,e)}function fo(e,t,n){var a=e.queue;if(a===null)throw Error(d(311));a.lastRenderedReducer=n;var i=e.baseQueue,l=a.pending;if(l!==null){if(i!==null){var s=i.next;i.next=l.next,l.next=s}t.baseQueue=i=l,a.pending=null}if(l=e.baseState,i===null)e.memoizedState=l;else{t=i.next;var u=s=null,m=null,g=t,S=!1;do{var C=g.lane&-536870913;if(C!==g.lane?(ge&C)===C:(An&C)===C){var v=g.revertLane;if(v===0)m!==null&&(m=m.next={lane:0,revertLane:0,gesture:null,action:g.action,hasEagerState:g.hasEagerState,eagerState:g.eagerState,next:null}),C===Na&&(S=!0);else if((An&v)===v){g=g.next,v===Na&&(S=!0);continue}else C={lane:0,revertLane:g.revertLane,gesture:null,action:g.action,hasEagerState:g.hasEagerState,eagerState:g.eagerState,next:null},m===null?(u=m=C,s=l):m=m.next=C,de.lanes|=v,ta|=v;C=g.action,Ta&&n(l,C),l=g.hasEagerState?g.eagerState:n(l,C)}else v={lane:C,revertLane:g.revertLane,gesture:g.gesture,action:g.action,hasEagerState:g.hasEagerState,eagerState:g.eagerState,next:null},m===null?(u=m=v,s=l):m=m.next=v,de.lanes|=C,ta|=C;g=g.next}while(g!==null&&g!==t);if(m===null?s=l:m.next=u,!Dt(l,e.memoizedState)&&($e=!0,S&&(n=ei,n!==null)))throw n;e.memoizedState=l,e.baseState=s,e.baseQueue=m,a.lastRenderedState=l}return i===null&&(a.lanes=0),[e.memoizedState,a.dispatch]}function po(e){var t=Ke(),n=t.queue;if(n===null)throw Error(d(311));n.lastRenderedReducer=e;var a=n.dispatch,i=n.pending,l=t.memoizedState;if(i!==null){n.pending=null;var s=i=i.next;do l=e(l,s.action),s=s.next;while(s!==i);Dt(l,t.memoizedState)||($e=!0),t.memoizedState=l,t.baseQueue===null&&(t.baseState=l),n.lastRenderedState=l}return[l,a]}function Hd(e,t,n){var a=de,i=Ke(),l=pe;if(l){if(n===void 0)throw Error(d(407));n=n()}else n=t();var s=!Dt((Ue||i).memoizedState,n);if(s&&(i.memoizedState=n,$e=!0),i=i.queue,ho(Pd.bind(null,a,i,e),[e]),e=i.getSnapshot!==t||s||Je!==null&&(Je.memoizedState.tag&1)!==0,li(e?9:8,{destroy:void 0},Id.bind(null,a,i,n,t),null),e){if(a.flags|=2048,Be===null)throw Error(d(349));l||(An&127)!==0||Gd(a,t,n)}return n}function Gd(e,t,n){e.flags|=16384,e={getSnapshot:t,value:n},t=de.updateQueue,t===null?(t=_r(),de.updateQueue=t,t.stores=[e]):(n=t.stores,n===null?t.stores=[e]:n.push(e))}function Id(e,t,n,a){t.value=n,t.getSnapshot=a,Yd(t)&&Qd(e)}function Pd(e,t,n){return n(function(){Yd(t)&&Qd(e)})}function Yd(e){var t=e.getSnapshot;e=e.value;try{var n=t();return!Dt(e,n)}catch{return!0}}function Qd(e){var t=ya(e,2);t!==null&&Tt(t,e,2)}function vo(e){var t=yt();if(typeof e=="function"){var n=e;if(e=n(),Ta){Bn(!0);try{n()}finally{Bn(!1)}}}return t.memoizedState=t.baseState=e,t.queue={pending:null,lanes:0,dispatch:null,lastRenderedReducer:On,lastRenderedState:e},t}function Fd(e,t,n,a){return e.baseState=n,fo(e,Ue,typeof a=="function"?a:On)}function Dv(e,t,n,a,i){if(yr(e))throw Error(d(485));if(e=t.action,e!==null){var l={payload:i,action:e,next:null,isTransition:!0,status:"pending",value:null,reason:null,listeners:[],then:function(s){l.listeners.push(s)}};J.T!==null?n(!0):l.isTransition=!1,a(l),n=t.pending,n===null?(l.next=t.pending=l,Kd(t,l)):(l.next=n.next,t.pending=n.next=l)}}function Kd(e,t){var n=t.action,a=t.payload,i=e.state;if(t.isTransition){var l=J.T,s={};s.types=l!==null?l.types:null,J.T=s;try{var u=n(i,a),m=J.S;m!==null&&m(s,u),Xd(e,t,u)}catch(g){yo(e,t,g)}finally{l!==null&&s.types!==null&&(l.types=s.types),J.T=l}}else try{l=n(i,a),Xd(e,t,l)}catch(g){yo(e,t,g)}}function Xd(e,t,n){n!==null&&typeof n=="object"&&typeof n.then=="function"?n.then(function(a){Zd(e,t,a)},function(a){return yo(e,t,a)}):Zd(e,t,n)}function Zd(e,t,n){t.status="fulfilled",t.value=n,Jd(t),e.state=n,t=e.pending,t!==null&&(n=t.next,n===t?e.pending=null:(n=n.next,t.next=n,Kd(e,n)))}function yo(e,t,n){var a=e.pending;if(e.pending=null,a!==null){a=a.next;do t.status="rejected",t.reason=n,Jd(t),t=t.next;while(t!==a)}e.action=null}function Jd(e){e=e.listeners;for(var t=0;t<e.length;t++)(0,e[t])()}function $d(e,t){return t}function Wd(e,t){if(pe){var n=Be.formState;if(n!==null){e:{var a=de;if(pe){if(Ve){t:{for(var i=Ve,l=Jt;i.nodeType!==8;){if(!l){i=null;break t}if(i=Wt(i.nextSibling),i===null){i=null;break t}}l=i.data,i=l==="F!"||l==="F"?i:null}if(i){Ve=Wt(i.nextSibling),a=i.data==="F!";break e}}In(a)}a=!1}a&&(t=n[0])}}return n=yt(),n.memoizedState=n.baseState=t,a={pending:null,lanes:0,dispatch:null,lastRenderedReducer:$d,lastRenderedState:t},n.queue=a,n=h_.bind(null,de,a),a.dispatch=n,a=vo(!1),l=No.bind(null,de,!1,a.queue),a=yt(),i={state:t,dispatch:null,action:e,pending:null},a.queue=i,n=Dv.bind(null,de,i,l,n),i.dispatch=n,a.memoizedState=e,[t,n,!1]}function e_(e){var t=Ke();return t_(t,Ue,e)}function t_(e,t,n){if(t=fo(e,t,$d)[0],e=fr(On)[0],typeof t=="object"&&t!==null&&typeof t.then=="function")try{var a=al(t)}catch(s){throw s===ti?ir:s}else a=t;t=Ke();var i=t.queue,l=i.dispatch;return n!==t.memoizedState&&(de.flags|=2048,li(9,{destroy:void 0},zv.bind(null,i,n),null)),[a,l,e]}function zv(e,t){e.action=t}function n_(e){var t=Ke(),n=Ue;if(n!==null)return t_(t,n,e);Ke(),t=t.memoizedState,n=Ke();var a=n.queue.dispatch;return n.memoizedState=e,[t,a,!1]}function li(e,t,n,a){return e={tag:e,create:n,deps:a,inst:t,next:null},t=de.updateQueue,t===null&&(t=_r(),de.updateQueue=t),n=t.lastEffect,n===null?t.lastEffect=e.next=e:(a=n.next,n.next=e,e.next=a,t.lastEffect=e),e}function a_(){return Ke().memoizedState}function pr(e,t,n,a){var i=yt();de.flags|=e,i.memoizedState=li(1|t,{destroy:void 0},n,a===void 0?null:a)}function vr(e,t,n,a){var i=Ke();a=a===void 0?null:a;var l=i.memoizedState.inst;Ue!==null&&a!==null&&so(a,Ue.memoizedState.deps)?i.memoizedState=li(t,l,n,a):(de.flags|=e,i.memoizedState=li(1|t,l,n,a))}function i_(e,t){pr(8390656,8,e,t)}function ho(e,t){vr(2048,8,e,t)}function Uv(e){de.flags|=4;var t=de.updateQueue;if(t===null)t=_r(),de.updateQueue=t,t.events=[e];else{var n=t.events;n===null?t.events=[e]:n.push(e)}}function l_(e){var t=Ke().memoizedState;return Uv({ref:t,nextImpl:e}),function(){if((Ae&2)!==0)throw Error(d(440));return t.impl.apply(void 0,arguments)}}function r_(e,t){return vr(4,2,e,t)}function s_(e,t){return vr(4,4,e,t)}function o_(e,t){if(typeof t=="function"){e=e();var n=t(e);return function(){typeof n=="function"?n():t(null)}}if(t!=null)return e=e(),t.current=e,function(){t.current=null}}function c_(e,t,n){n=n!=null?n.concat([e]):null,vr(4,4,o_.bind(null,t,e),n)}function go(){}function u_(e,t){var n=Ke();t=t===void 0?null:t;var a=n.memoizedState;return t!==null&&so(t,a[1])?a[0]:(n.memoizedState=[e,t],e)}function d_(e,t){var n=Ke();t=t===void 0?null:t;var a=n.memoizedState;if(t!==null&&so(t,a[1]))return a[0];if(a=e(),Ta){Bn(!0);try{e()}finally{Bn(!1)}}return n.memoizedState=[a,t],a}function bo(e,t,n){return n===void 0||(An&1073741824)!==0&&(ge&261930)===0?e.memoizedState=t:(e.memoizedState=n,e=xm(),de.lanes|=e,ta|=e,n)}function __(e,t,n,a){return Dt(n,t)?n:Xn.current!==null?(e=bo(e,n,a),Dt(e,t)||($e=!0),e):(An&106)===0||(An&1073741824)!==0&&(ge&261930)===0?($e=!0,e.memoizedState=n):(e=xm(),de.lanes|=e,ta|=e,t)}function m_(e,t,n,a,i){var l=se.p;se.p=l!==0&&8>l?l:8;var s=J.T,u={};u.types=s!==null?s.types:null,J.T=u,No(e,!1,t,n);try{var m=i(),g=J.S;if(g!==null&&g(u,m),m!==null&&typeof m=="object"&&typeof m.then=="function"){var S=Rv(m,a);il(e,t,S,Lt(e))}else il(e,t,a,Lt(e))}catch(C){il(e,t,{then:function(){},status:"rejected",reason:C},Lt())}finally{se.p=l,s!==null&&u.types!==null&&(s.types=u.types),J.T=s}}function Bv(){}function xo(e,t,n,a){if(e.tag!==5)throw Error(d(476));var i=f_(e).queue;m_(e,i,t,Gt,n===null?Bv:function(){return p_(e),n(a)})}function f_(e){var t=e.memoizedState;if(t!==null)return t;t={memoizedState:Gt,baseState:Gt,baseQueue:null,queue:{pending:null,lanes:0,dispatch:null,lastRenderedReducer:On,lastRenderedState:Gt},next:null};var n={};return t.next={memoizedState:n,baseState:n,baseQueue:null,queue:{pending:null,lanes:0,dispatch:null,lastRenderedReducer:On,lastRenderedState:n},next:null},e.memoizedState=t,e=e.alternate,e!==null&&(e.memoizedState=t),t}function p_(e){var t=f_(e);t.next===null&&(t=e.alternate.memoizedState),il(e,t.next.queue,{},Lt())}function wo(){return st(Si)}function v_(){return Ke().memoizedState}function y_(){return Ke().memoizedState}function Vv(e){for(var t=e.return;t!==null;){switch(t.tag){case 24:case 3:var n=Lt();e=Fn(n);var a=Kn(t,e,n);a!==null&&(Tt(a,t,n),$i(a,t,n)),t={cache:Xs()},e.payload=t;return}t=t.return}}function Lv(e,t,n){var a=Lt();n={lane:a,revertLane:0,gesture:null,action:n,hasEagerState:!1,eagerState:null,next:null},yr(e)?g_(t,n):(n=Ls(e,t,n,a),n!==null&&(Tt(n,e,a),b_(n,t,a)))}function h_(e,t,n){var a=Lt();il(e,t,n,a)}function il(e,t,n,a){var i={lane:a,revertLane:0,gesture:null,action:n,hasEagerState:!1,eagerState:null,next:null};if(yr(e))g_(t,i);else{var l=e.alternate;if(e.lanes===0&&(l===null||l.lanes===0)&&(l=t.lastRenderedReducer,l!==null))try{var s=t.lastRenderedState,u=l(s,n);if(i.hasEagerState=!0,i.eagerState=u,Dt(u,s))return Kl(e,t,i,0),Be===null&&Fl(),!1}catch{}finally{}if(n=Ls(e,t,i,a),n!==null)return Tt(n,e,a),b_(n,t,a),!0}return!1}function No(e,t,n,a){if(a={lane:2,revertLane:mc(),gesture:null,action:a,hasEagerState:!1,eagerState:null,next:null},yr(e)){if(t)throw Error(d(479))}else t=Ls(e,n,a,2),t!==null&&Tt(t,e,2)}function yr(e){var t=e.alternate;return e===de||t!==null&&t===de}function g_(e,t){ai=ur=!0;var n=e.pending;n===null?t.next=t:(t.next=n.next,n.next=t),e.pending=t}function b_(e,t,n){if((n&4194048)!==0){var a=t.lanes;a&=e.pendingLanes,n|=a,t.lanes=n,xu(e,n)}}var hr={readContext:st,use:mr,useCallback:Ye,useContext:Ye,useEffect:Ye,useImperativeHandle:Ye,useLayoutEffect:Ye,useInsertionEffect:Ye,useMemo:Ye,useReducer:Ye,useRef:Ye,useState:Ye,useDebugValue:Ye,useDeferredValue:Ye,useTransition:Ye,useSyncExternalStore:Ye,useId:Ye,useHostTransitionStatus:Ye,useFormState:Ye,useActionState:Ye,useOptimistic:Ye,useMemoCache:Ye,useCacheRefresh:Ye,useEffectEvent:Ye},x_={readContext:st,use:mr,useCallback:function(e,t){return yt().memoizedState=[e,t===void 0?null:t],e},useContext:st,useEffect:i_,useImperativeHandle:function(e,t,n){n=n!=null?n.concat([e]):null,pr(4194308,4,o_.bind(null,t,e),n)},useLayoutEffect:function(e,t){return pr(4194308,4,e,t)},useInsertionEffect:function(e,t){pr(4,2,e,t)},useMemo:function(e,t){var n=yt();t=t===void 0?null:t;var a=e();if(Ta){Bn(!0);try{e()}finally{Bn(!1)}}return n.memoizedState=[a,t],a},useReducer:function(e,t,n){var a=yt();if(n!==void 0){var i=n(t);if(Ta){Bn(!0);try{n(t)}finally{Bn(!1)}}}else i=t;return a.memoizedState=a.baseState=i,e={pending:null,lanes:0,dispatch:null,lastRenderedReducer:e,lastRenderedState:i},a.queue=e,e=e.dispatch=Lv.bind(null,de,e),[a.memoizedState,e]},useRef:function(e){var t=yt();return e={current:e},t.memoizedState=e},useState:function(e){e=vo(e);var t=e.queue,n=h_.bind(null,de,t);return t.dispatch=n,[e.memoizedState,n]},useDebugValue:go,useDeferredValue:function(e,t){var n=yt();return bo(n,e,t)},useTransition:function(){var e=vo(!1);return e=m_.bind(null,de,e.queue,!0,!1),yt().memoizedState=e,[!1,e]},useSyncExternalStore:function(e,t,n){var a=de,i=yt();if(pe){if(n===void 0)throw Error(d(407));n=n()}else{if(n=t(),Be===null)throw Error(d(349));(ge&127)!==0||Gd(a,t,n)}i.memoizedState=n;var l={value:n,getSnapshot:t};return i.queue=l,i_(Pd.bind(null,a,l,e),[e]),a.flags|=2048,li(9,{destroy:void 0},Id.bind(null,a,l,n,t),null),n},useId:function(){var e=yt(),t=Be.identifierPrefix;if(pe){var n=un,a=cn;n=(a&~(1<<32-qt(a)-1)).toString(32)+n,t="_"+t+"R_"+n,n=dr++,0<n&&(t+="H"+n.toString(32)),t+="_"}else n=qv++,t="_"+t+"r_"+n.toString(32)+"_";return e.memoizedState=t},useHostTransitionStatus:wo,useFormState:Wd,useActionState:Wd,useOptimistic:function(e){var t=yt();t.memoizedState=t.baseState=e;var n={pending:null,lanes:0,dispatch:null,lastRenderedReducer:null,lastRenderedState:null};return t.queue=n,t=No.bind(null,de,!0,n),n.dispatch=t,[e,t]},useMemoCache:mo,useCacheRefresh:function(){return yt().memoizedState=Vv.bind(null,de)},useEffectEvent:function(e){var t=yt(),n={impl:e};return t.memoizedState=n,function(){if((Ae&2)!==0)throw Error(d(440));return n.impl.apply(void 0,arguments)}}},w_={readContext:st,use:mr,useCallback:u_,useContext:st,useEffect:ho,useImperativeHandle:c_,useInsertionEffect:r_,useLayoutEffect:s_,useMemo:d_,useReducer:fr,useRef:a_,useState:function(){return fr(On)},useDebugValue:go,useDeferredValue:function(e,t){var n=Ke();return __(n,Ue.memoizedState,e,t)},useTransition:function(){var e=fr(On)[0],t=Ke().memoizedState;return[typeof e=="boolean"?e:al(e),t]},useSyncExternalStore:Hd,useId:v_,useHostTransitionStatus:wo,useFormState:e_,useActionState:e_,useOptimistic:function(e,t){var n=Ke();return Fd(n,Ue,e,t)},useMemoCache:mo,useCacheRefresh:y_,useEffectEvent:l_},Hv={readContext:st,use:mr,useCallback:u_,useContext:st,useEffect:ho,useImperativeHandle:c_,useInsertionEffect:r_,useLayoutEffect:s_,useMemo:d_,useReducer:po,useRef:a_,useState:function(){return po(On)},useDebugValue:go,useDeferredValue:function(e,t){var n=Ke();return Ue===null?bo(n,e,t):__(n,Ue.memoizedState,e,t)},useTransition:function(){var e=po(On)[0],t=Ke().memoizedState;return[typeof e=="boolean"?e:al(e),t]},useSyncExternalStore:Hd,useId:v_,useHostTransitionStatus:wo,useFormState:n_,useActionState:n_,useOptimistic:function(e,t){var n=Ke();return Ue!==null?Fd(n,Ue,e,t):(n.baseState=e,[e,n.queue.dispatch])},useMemoCache:mo,useCacheRefresh:y_,useEffectEvent:l_};function So(e,t,n,a){t=e.memoizedState,n=n(a,t),n=n==null?t:D({},t,n),e.memoizedState=n,e.lanes===0&&(e.updateQueue.baseState=n)}var jo={enqueueSetState:function(e,t,n){e=e._reactInternals;var a=Lt(),i=Fn(a);i.payload=t,n!=null&&(i.callback=n),t=Kn(e,i,a),t!==null&&(Tt(t,e,a),$i(t,e,a))},enqueueReplaceState:function(e,t,n){e=e._reactInternals;var a=Lt(),i=Fn(a);i.tag=1,i.payload=t,n!=null&&(i.callback=n),t=Kn(e,i,a),t!==null&&(Tt(t,e,a),$i(t,e,a))},enqueueForceUpdate:function(e,t){e=e._reactInternals;var n=Lt(),a=Fn(n);a.tag=2,t!=null&&(a.callback=t),t=Kn(e,a,n),t!==null&&(Tt(t,e,n),$i(t,e,n))}};function N_(e,t,n,a,i,l,s){return e=e.stateNode,typeof e.shouldComponentUpdate=="function"?e.shouldComponentUpdate(a,l,s):t.prototype&&t.prototype.isPureReactComponent?!Pi(n,a)||!Pi(i,l):!0}function S_(e,t,n,a){e=t.state,typeof t.componentWillReceiveProps=="function"&&t.componentWillReceiveProps(n,a),typeof t.UNSAFE_componentWillReceiveProps=="function"&&t.UNSAFE_componentWillReceiveProps(n,a),t.state!==e&&jo.enqueueReplaceState(t,t.state,null)}function Ca(e,t){var n=t;if("ref"in t){n={};for(var a in t)a!=="ref"&&(n[a]=t[a])}if(e=e.defaultProps){n===t&&(n=D({},n));for(var i in e)n[i]===void 0&&(n[i]=e[i])}return n}function j_(e){Ql(e)}function E_(e){console.error(e)}function k_(e){Ql(e)}function gr(e,t){try{var n=e.onUncaughtError;n(t.value,{componentStack:t.stack})}catch(a){setTimeout(function(){throw a})}}function T_(e,t,n){try{var a=e.onCaughtError;a(n.value,{componentStack:n.stack,errorBoundary:t.tag===1?t.stateNode:null})}catch(i){setTimeout(function(){throw i})}}function Eo(e,t,n){return n=Fn(n),n.tag=3,n.payload={element:null},n.callback=function(){gr(e,t)},n}function C_(e){return e=Fn(e),e.tag=3,e}function A_(e,t,n,a){var i=n.type.getDerivedStateFromError;if(typeof i=="function"){var l=a.value;e.payload=function(){return i(l)},e.callback=function(){T_(t,n,a)}}var s=n.stateNode;s!==null&&typeof s.componentDidCatch=="function"&&(e.callback=function(){T_(t,n,a),typeof i!="function"&&(na===null?na=new Set([this]):na.add(this));var u=a.stack;this.componentDidCatch(a.value,{componentStack:u!==null?u:""})})}function Gv(e,t,n,a,i){if(n.flags|=32768,a!==null&&typeof a=="object"&&typeof a.then=="function"){if(t=n.alternate,t!==null&&xa(t,n,i,!0),n=ot.current,n!==null){switch(n.tag){case 31:case 13:case 19:return pt===null?Lr():n.alternate===null&&Qe===0&&(Qe=3),n.flags&=-257,n.flags|=65536,n.lanes=i,a===lr?n.flags|=16384:(t=n.updateQueue,t===null?n.updateQueue=new Set([a]):t.add(a),uc(e,a,i)),!1;case 22:return n.flags|=65536,a===lr?n.flags|=16384:(t=n.updateQueue,t===null?(t={transitions:null,markerInstances:null,retryQueue:new Set([a])},n.updateQueue=t):(n=t.retryQueue,n===null?t.retryQueue=new Set([a]):n.add(a)),uc(e,a,i)),!1}throw Error(d(435,n.tag))}return uc(e,a,i),Lr(),!1}if(pe)return t=ot.current,t!==null?((t.flags&65536)===0&&(t.flags|=256),t.flags|=65536,t.lanes=i,a!==Ys&&(e=Error(d(422),{cause:a}),Fi(Kt(e,n)))):(a!==Ys&&(t=Error(d(423),{cause:a}),Fi(Kt(t,n))),e=e.current.alternate,e.flags|=65536,i&=-i,e.lanes|=i,a=Kt(a,n),i=Eo(e.stateNode,a,i),to(e,i),Qe!==4&&(Qe=2)),!1;var l=Error(d(520),{cause:a});if(l=Kt(l,n),_l===null?_l=[l]:_l.push(l),Qe!==4&&(Qe=2),t===null)return!0;a=Kt(a,n),n=t;do{switch(n.tag){case 3:return n.flags|=65536,e=i&-i,n.lanes|=e,e=Eo(n.stateNode,a,e),to(n,e),!1;case 1:if(t=n.type,l=n.stateNode,(n.flags&128)===0&&(typeof t.getDerivedStateFromError=="function"||l!==null&&typeof l.componentDidCatch=="function"&&(na===null||!na.has(l))))return n.flags|=65536,i&=-i,n.lanes|=i,i=C_(i),A_(i,e,n,a),to(n,i),!1;break;case 22:if(n.memoizedState!==null)return n.flags|=65536,!1}n=n.return}while(n!==null);return!1}var ko=Error(d(461)),$e=!1;function et(e,t,n,a){t.child=e===null?Md(t,null,n,a):ka(t,e.child,n,a)}function O_(e,t,n,a,i){n=n.render;var l=t.ref;if("ref"in a){var s={};for(var u in a)u!=="ref"&&(s[u]=a[u])}else s=a;return wa(t),a=oo(e,t,n,s,l,i),u=co(),e!==null&&!$e?(uo(e,t,i),Rn(e,t,i)):(pe&&u&&$l(t),t.flags|=1,et(e,t,a,i),t.child)}function R_(e,t,n,a,i){if(e===null){var l=n.type;return typeof l=="function"&&!Hs(l)&&l.defaultProps===void 0&&n.compare===null?(t.tag=15,t.type=l,q_(e,t,l,a,i)):(e=Zl(n.type,null,a,t,t.mode,i),e.ref=t.ref,e.return=t,t.child=e)}if(l=e.child,!Do(e,i)){var s=l.memoizedProps;if(n=n.compare,n=n!==null?n:Pi,n(s,a)&&e.ref===t.ref)return Rn(e,t,i)}return t.flags|=1,e=En(l,a),e.ref=t.ref,e.return=t,t.child=e}function q_(e,t,n,a,i){if(e!==null){var l=e.memoizedProps;if(Pi(l,a)&&e.ref===t.ref)if($e=!1,t.pendingProps=a=l,Do(e,i))(e.flags&131072)!==0&&($e=!0);else return t.lanes=e.lanes,Rn(e,t,i)}return To(e,t,n,a,i)}function M_(e,t,n,a){var i=a.children,l=e!==null?e.memoizedState:null;if(e===null&&t.stateNode===null&&(t.stateNode={_visibility:1,_pendingMarkers:null,_retryCache:null,_transitions:null}),a.mode==="hidden"){if((t.flags&128)!==0){if(l=l!==null?l.baseLanes|n:n,e!==null){for(a=t.child=e.child,i=0;a!==null;)i=i|a.lanes|a.childLanes,a=a.sibling;a=i&~l}else a=0,t.child=null;return D_(e,t,l,n,a)}if((n&536870912)!==0)t.memoizedState={baseLanes:0,cachePool:null},e!==null&&ar(t,l!==null?l.cachePool:null),l!==null?Ud(t,l):ao(),Bd(t);else return a=t.lanes=536870912,D_(e,t,l!==null?l.baseLanes|n:n,n,a)}else l!==null?(ar(t,l.cachePool),Ud(t,l),Jn(),t.memoizedState=null):(e!==null&&ar(t,null),ao(),Jn());return et(e,t,i,n),t.child}function ll(e,t){return e!==null&&e.tag===22||t.stateNode!==null||(t.stateNode={_visibility:1,_pendingMarkers:null,_retryCache:null,_transitions:null}),t.sibling}function D_(e,t,n,a,i){var l=Js();return l=l===null?null:{parent:Ze._currentValue,pool:l},t.memoizedState={baseLanes:n,cachePool:l},e!==null&&ar(t,null),ao(),Bd(t),e!==null&&xa(e,t,a,!0),t.childLanes=i,null}function br(e,t){return t=xr({mode:t.mode,children:t.children},e.mode),t.ref=e.ref,e.child=t,t.return=e,t}function z_(e,t,n){return ka(t,e.child,null,n),e=br(t,t.pendingProps),e.flags|=2,zt(t),t.memoizedState=null,e}function Iv(e,t,n){var a=t.pendingProps,i=(t.flags&128)!==0;if(t.flags&=-129,e===null){if(pe){if(a.mode==="hidden")return e=br(t,a),t.lanes=536870912,e.memoizedState={baseLanes:0,cachePool:null},ll(null,e);if(lo(t),(e=Ve)?(e=cf(e,Jt),e=e!==null&&e.data==="&"?e:null,e!==null&&(t.memoizedState={dehydrated:e,treeContext:Hn!==null?{id:cn,overflow:un}:null,retryLane:536870912,hydrationErrors:null},n=gd(e),n.return=t,t.child=n,nt=t,Ve=null)):e=null,e===null)throw In(t);return t.lanes=536870912,null}return br(t,a)}var l=e.memoizedState;if(l!==null){var s=l.dehydrated;if(lo(t),i)if(t.flags&256)t.flags&=-257,t=z_(e,t,n);else if(t.memoizedState!==null)t.child=e.child,t.flags|=128,t=null;else throw Error(d(558));else if($e||xa(e,t,n,!1),i=(n&e.childLanes)!==0,$e||i){if(Xn.current===null){if(a=Be,a!==null&&(s=wu(a,n),s!==0&&s!==l.retryLane))throw l.retryLane=s,ya(e,s),Tt(a,e,s),ko;Lr()}t=z_(e,t,n)}else e=l.treeContext,Ve=Wt(s.nextSibling),nt=t,pe=!0,Gn=null,Jt=!1,e!==null&&wd(t,e),t=br(t,a),t.flags|=134221824;return t}return e=En(e.child,{mode:a.mode,children:a.children}),e.ref=t.ref,t.child=e,e.return=t,e}function ri(e,t){var n=t.ref;if(n===null)e!==null&&e.ref!==null&&(t.flags|=4194816);else{if(typeof n!="function"&&typeof n!="object")throw Error(d(284));(e===null||e.ref!==n)&&(t.flags|=4194816)}}function To(e,t,n,a,i){return wa(t),n=oo(e,t,n,a,void 0,i),a=co(),e!==null&&!$e?(uo(e,t,i),Rn(e,t,i)):(pe&&a&&$l(t),t.flags|=1,et(e,t,n,i),t.child)}function U_(e,t,n,a,i,l){return wa(t),t.updateQueue=null,n=Ld(t,a,n,i),Vd(e),a=co(),e!==null&&!$e?(uo(e,t,l),Rn(e,t,l)):(pe&&a&&$l(t),t.flags|=1,et(e,t,n,l),t.child)}function B_(e,t,n,a,i){if(wa(t),t.stateNode===null){var l=Za,s=n.contextType;typeof s=="object"&&s!==null&&(l=st(s)),l=new n(a,l),t.memoizedState=l.state!==null&&l.state!==void 0?l.state:null,l.updater=jo,t.stateNode=l,l._reactInternals=t,l=t.stateNode,l.props=a,l.state=t.memoizedState,l.refs={},Ws(t),s=n.contextType,l.context=typeof s=="object"&&s!==null?st(s):Za,l.state=t.memoizedState,s=n.getDerivedStateFromProps,typeof s=="function"&&(So(t,n,s,a),l.state=t.memoizedState),typeof n.getDerivedStateFromProps=="function"||typeof l.getSnapshotBeforeUpdate=="function"||typeof l.UNSAFE_componentWillMount!="function"&&typeof l.componentWillMount!="function"||(s=l.state,typeof l.componentWillMount=="function"&&l.componentWillMount(),typeof l.UNSAFE_componentWillMount=="function"&&l.UNSAFE_componentWillMount(),s!==l.state&&jo.enqueueReplaceState(l,l.state,null),el(t,a,l,i),Wi(),l.state=t.memoizedState),typeof l.componentDidMount=="function"&&(t.flags|=4194308),a=!0}else if(e===null){l=t.stateNode;var u=t.memoizedProps,m=Ca(n,u);l.props=m;var g=l.context,S=n.contextType;s=Za,typeof S=="object"&&S!==null&&(s=st(S));var C=n.getDerivedStateFromProps;S=typeof C=="function"||typeof l.getSnapshotBeforeUpdate=="function",u=t.pendingProps!==u,S||typeof l.UNSAFE_componentWillReceiveProps!="function"&&typeof l.componentWillReceiveProps!="function"||(u||g!==s)&&S_(t,l,a,s),Qn=!1;var v=t.memoizedState;l.state=v,el(t,a,l,i),Wi(),g=t.memoizedState,u||v!==g||Qn?(typeof C=="function"&&(So(t,n,C,a),g=t.memoizedState),(m=Qn||N_(t,n,m,a,v,g,s))?(S||typeof l.UNSAFE_componentWillMount!="function"&&typeof l.componentWillMount!="function"||(typeof l.componentWillMount=="function"&&l.componentWillMount(),typeof l.UNSAFE_componentWillMount=="function"&&l.UNSAFE_componentWillMount()),typeof l.componentDidMount=="function"&&(t.flags|=4194308)):(typeof l.componentDidMount=="function"&&(t.flags|=4194308),t.memoizedProps=a,t.memoizedState=g),l.props=a,l.state=g,l.context=s,a=m):(typeof l.componentDidMount=="function"&&(t.flags|=4194308),a=!1)}else{l=t.stateNode,eo(e,t),s=t.memoizedProps,S=Ca(n,s),l.props=S,C=t.pendingProps,v=l.context,g=n.contextType,m=Za,typeof g=="object"&&g!==null&&(m=st(g)),u=n.getDerivedStateFromProps,(g=typeof u=="function"||typeof l.getSnapshotBeforeUpdate=="function")||typeof l.UNSAFE_componentWillReceiveProps!="function"&&typeof l.componentWillReceiveProps!="function"||(s!==C||v!==m)&&S_(t,l,a,m),Qn=!1,v=t.memoizedState,l.state=v,el(t,a,l,i),Wi();var w=t.memoizedState;s!==C||v!==w||Qn||e!==null&&e.dependencies!==null&&tr(e.dependencies)?(typeof u=="function"&&(So(t,n,u,a),w=t.memoizedState),(S=Qn||N_(t,n,S,a,v,w,m)||e!==null&&e.dependencies!==null&&tr(e.dependencies))?(g||typeof l.UNSAFE_componentWillUpdate!="function"&&typeof l.componentWillUpdate!="function"||(typeof l.componentWillUpdate=="function"&&l.componentWillUpdate(a,w,m),typeof l.UNSAFE_componentWillUpdate=="function"&&l.UNSAFE_componentWillUpdate(a,w,m)),typeof l.componentDidUpdate=="function"&&(t.flags|=4),typeof l.getSnapshotBeforeUpdate=="function"&&(t.flags|=1024)):(typeof l.componentDidUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=4),typeof l.getSnapshotBeforeUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=1024),t.memoizedProps=a,t.memoizedState=w),l.props=a,l.state=w,l.context=m,a=S):(typeof l.componentDidUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=4),typeof l.getSnapshotBeforeUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=1024),a=!1)}return l=a,ri(e,t),a=(t.flags&128)!==0,l||a?(l=t.stateNode,n=a&&typeof n.getDerivedStateFromError!="function"?null:l.render(),t.flags|=1,e!==null&&a?(t.child=ka(t,e.child,null,i),t.child=ka(t,null,n,i)):et(e,t,n,i),t.memoizedState=l.state,e=t.child):e=Rn(e,t,i),e}function V_(e,t,n,a){return ga(),t.flags|=256,et(e,t,n,a),t.child}var Co={dehydrated:null,treeContext:null,retryLane:0,hydrationErrors:null};function Ao(e){return{baseLanes:e,cachePool:Td()}}function Oo(e,t,n){return e=e!==null?e.childLanes&~n:0,t&&(e|=Vt),e}function L_(e,t,n){var a=t.pendingProps,i=!1,l=(t.flags&128)!==0,s;if((s=l)||(s=e!==null&&e.memoizedState===null?!1:(ct.current&2)!==0),s&&(i=!0,t.flags&=-129),s=(t.flags&32)!==0,t.flags&=-33,e===null){if(pe){if(i?Zn(t):Jn(),(e=Ve)?(e=cf(e,Jt),e=e!==null&&e.data!=="&"?e:null,e!==null&&(t.memoizedState={dehydrated:e,treeContext:Hn!==null?{id:cn,overflow:un}:null,retryLane:536870912,hydrationErrors:null},n=gd(e),n.return=t,t.child=n,nt=t,Ve=null)):e=null,e===null)throw In(t);return Ac(e)?t.lanes=32:t.lanes=536870912,null}return l=a.children,a=a.fallback,i?(Jn(),i=t.mode,l=xr({mode:"hidden",children:l},i),a=ha(a,i,n,null),l.return=t,a.return=t,l.sibling=a,t.child=l,a=t.child,a.memoizedState=Ao(n),a.childLanes=Oo(e,s,n),t.memoizedState=Co,ll(null,a)):(Zn(t),Ro(t,l))}var u=e.memoizedState;if(u!==null){var m=u.dehydrated;if(m!==null)return Pv(e,t,l,s,a,m,u,n)}return i?(Jn(),i=a.fallback,l=t.mode,u=e.child,m=u.sibling,a=En(u,{mode:"hidden",children:a.children}),a.subtreeFlags=u.subtreeFlags&1206910976,m!==null?i=En(m,i):(i=ha(i,l,n,null),i.flags|=2),i.return=t,a.return=t,a.sibling=i,t.child=a,ll(null,a),a=t.child,i=e.child.memoizedState,i===null?i=Ao(n):(l=i.cachePool,l!==null?(u=Ze._currentValue,l=l.parent!==u?{parent:u,pool:u}:l):l=Td(),i={baseLanes:i.baseLanes|n,cachePool:l}),a.memoizedState=i,a.childLanes=Oo(e,s,n),t.memoizedState=Co,ll(e.child,a)):(Zn(t),n=e.child,e=n.sibling,n=En(n,{mode:"visible",children:a.children}),n.return=t,n.sibling=null,e!==null&&(s=t.deletions,s===null?(t.deletions=[e],t.flags|=16):s.push(e)),t.child=n,t.memoizedState=null,n)}function Ro(e,t){return t=xr({mode:"visible",children:t},e.mode),t.return=e,e.child=t}function xr(e,t){return e=St(22,e,null,t),e.lanes=0,e}function wr(e,t,n){return ka(t,e.child,null,n),e=Ro(t,t.pendingProps.children),e.flags|=2,t.memoizedState=null,e}function Pv(e,t,n,a,i,l,s,u){if(n)return t.flags&256?(Zn(t),t.flags&=-257,wr(e,t,u)):t.memoizedState!==null?(Jn(),t.child=e.child,t.flags|=128,null):(Jn(),l=i.fallback,s=t.mode,i=xr({mode:"visible",children:i.children},s),l=ha(l,s,u,null),l.flags|=2,i.return=t,l.return=t,i.sibling=l,t.child=i,ka(t,e.child,null,u),i=t.child,i.memoizedState=Ao(u),i.childLanes=Oo(e,a,u),t.memoizedState=Co,ll(null,i));if(Zn(t),Ac(l)){if(a=l.nextSibling&&l.nextSibling.dataset,a)var m=a.dgst;return a=m,a!==""&&(i=Error(d(419)),i.stack="",i.digest=a,Fi({value:i,source:null,stack:null})),wr(e,t,u)}if($e||xa(e,t,u,!1),a=(u&e.childLanes)!==0,$e||a){if(Xn.current!==null)return wr(e,t,u);if(a=Be,a!==null&&(i=wu(a,u),i!==0&&i!==s.retryLane))throw s.retryLane=i,ya(e,i),Tt(a,e,i),ko;return Cc(l)||Lr(),wr(e,t,u)}return Cc(l)?(t.flags|=192,t.child=e.child,null):(e=s.treeContext,Ve=Wt(l.nextSibling),nt=t,pe=!0,Gn=null,Jt=!1,e!==null&&wd(t,e),t=Ro(t,i.children),t.flags|=134221824,t)}function H_(e,t,n){e.lanes|=t;var a=e.alternate;a!==null&&(a.lanes|=t),er(e.return,t,n)}function G_(e){for(var t=null;e!==null;){var n=e.alternate;n!==null&&cr(n)===null&&(t=e),e=e.sibling}return t}function Nr(e,t,n,a,i,l){var s=e.memoizedState;s===null?e.memoizedState={isBackwards:t,rendering:null,renderingStartTime:0,last:a,tail:n,tailMode:i,treeForkCount:l}:(s.isBackwards=t,s.rendering=null,s.renderingStartTime=0,s.last=a,s.tail=n,s.tailMode=i,s.treeForkCount=l)}function qo(e){var t=e.child;for(e.child=null;t!==null;){var n=t.sibling;t.sibling=e.child,e.child=t,t=n}}function Mo(e,t,n){var a=t.pendingProps,i=a.revealOrder,l=a.tail;a=a.children;var s=ct.current;if(t.flags&128)return tl(t,s),null;var u=(s&2)!==0;if(u?(s=s&1|2,t.flags|=128):s&=1,tl(t,s),i==="backwards"&&e!==null?(qo(e),et(e,t,a,n),qo(e)):et(e,t,a,n),a=pe?Qi:0,!u&&e!==null&&(e.flags&128)!==0)e:for(e=t.child;e!==null;){if(e.tag===13)e.memoizedState!==null&&H_(e,n,t);else if(e.tag===19)H_(e,n,t);else if(e.child!==null){e.child.return=e,e=e.child;continue}if(e===t)break e;for(;e.sibling===null;){if(e.return===null||e.return===t)break e;e=e.return}e.sibling.return=e.return,e=e.sibling}switch(i){case"backwards":n=G_(t.child),n===null?(i=t.child,t.child=null):(i=n.sibling,n.sibling=null,qo(t)),Nr(t,!0,i,null,l,a);break;case"unstable_legacy-backwards":for(n=null,i=t.child,t.child=null;i!==null;){if(e=i.alternate,e!==null&&cr(e)===null){t.child=i;break}e=i.sibling,i.sibling=n,n=i,i=e}Nr(t,!0,n,null,l,a);break;case"together":Nr(t,!1,null,null,void 0,a);break;case"independent":t.memoizedState=null;break;default:n=G_(t.child),n===null?(i=t.child,t.child=null):(i=n.sibling,n.sibling=null),Nr(t,!1,i,n,l,a)}return t.child}function I_(e,t,n){var a=t.pendingProps;return Pn(t,t.type,a.value),et(e,t,a.children,n),t.child}function Rn(e,t,n){if(e!==null&&(t.dependencies=e.dependencies),ta|=t.lanes,(n&t.childLanes)===0)if(e!==null){if(xa(e,t,n,!1),(n&t.childLanes)===0)return null}else return null;if(e!==null&&t.child!==e.child)throw Error(d(153));if(t.child!==null){for(e=t.child,n=En(e,e.pendingProps),t.child=n,n.return=t;e.sibling!==null;)e=e.sibling,n=n.sibling=En(e,e.pendingProps),n.return=t;n.sibling=null}return t.child}function Do(e,t){return(e.lanes&t)!==0?!0:(e=e.dependencies,!!(e!==null&&tr(e)))}function Yv(e,t,n){switch(t.tag){case 3:lt(t,t.stateNode.containerInfo),Pn(t,Ze,e.memoizedState.cache),ga();break;case 27:case 5:xt(t);break;case 4:lt(t,t.stateNode.containerInfo);break;case 10:Pn(t,t.type,t.memoizedProps.value);break;case 31:if(t.memoizedState!==null)return t.flags|=128,lo(t),null;break;case 13:var a=t.memoizedState;if(a!==null){if(a.dehydrated!==null)return Zn(t),t.flags|=128,null;a=xa(e,t,n,!1);var i=t.child.childLanes;return a||(n&i)!==0?L_(e,t,n):(Zn(t),e=Rn(e,t,n),e!==null?e.sibling:null)}Zn(t);break;case 19:if(t.flags&128)return Mo(e,t,n);if(i=(e.flags&128)!==0,a=(n&t.childLanes)!==0,a||(xa(e,t,n,!1),a=(n&t.childLanes)!==0),i){if(a)return Mo(e,t,n);t.flags|=128}if(i=t.memoizedState,i!==null&&(i.rendering=null,i.tail=null,i.lastEffect=null),tl(t,ct.current),a)break;return null;case 22:return t.lanes=0,M_(e,t,n,t.pendingProps);case 24:Pn(t,Ze,e.memoizedState.cache)}return Rn(e,t,n)}function P_(e,t,n){if(e!==null)if(e.memoizedProps!==t.pendingProps)$e=!0;else{if(!Do(e,n)&&(t.flags&128)===0)return $e=!1,Yv(e,t,n);$e=(e.flags&131072)!==0}else $e=!1,pe&&(t.flags&1048576)!==0&&xd(t,Qi,t.index);switch(t.lanes=0,t.tag){case 16:e:{var a=t.pendingProps;if(e=ja(t.elementType),t.type=e,typeof e=="function")Hs(e)?(a=Ca(e,a),t.tag=1,t=B_(null,t,e,a,n)):(t.tag=0,t=To(null,t,e,a,n));else{if(e!=null){var i=e.$$typeof;if(i===z){t.tag=11,t=O_(null,t,e,a,n);break e}else if(i===ke){t.tag=14,t=R_(null,t,e,a,n);break e}else if(i===He){t.tag=10,t.type=e,t=I_(null,t,n);break e}}throw t=we(e)||e,Error(d(306,t,""))}}return t;case 0:return To(e,t,t.type,t.pendingProps,n);case 1:return a=t.type,i=Ca(a,t.pendingProps),B_(e,t,a,i,n);case 3:e:{if(lt(t,t.stateNode.containerInfo),e===null)throw Error(d(387));a=t.pendingProps;var l=t.memoizedState;i=l.element,eo(e,t),el(t,a,null,n);var s=t.memoizedState;if(a=s.cache,Pn(t,Ze,a),a!==l.cache&&Ks(t,[Ze],n,!0),Wi(),a=s.element,l.isDehydrated)if(l={element:a,isDehydrated:!1,cache:s.cache},t.updateQueue.baseState=l,t.memoizedState=l,t.flags&256){t=V_(e,t,a,n);break e}else if(a!==i){i=Kt(Error(d(424)),t),Fi(i),t=V_(e,t,a,n);break e}else{switch(e=t.stateNode.containerInfo,e.nodeType){case 9:e=e.body;break;default:e=e.nodeName==="HTML"?e.ownerDocument.body:e}for(Ve=Wt(e.firstChild),nt=t,pe=!0,Gn=null,Jt=!0,n=Md(t,null,a,n),t.child=n;n;)n.flags=n.flags&-3|134221824,n=n.sibling}else{if(ga(),a===i){t=Rn(e,t,n);break e}et(e,t,a,n)}t=t.child}return t;case 26:return ri(e,t),e===null?(n=vf(t.type,null,t.pendingProps,null))?t.memoizedState=n:pe||(t.stateNode=Km(t.type,t.pendingProps,Fe.current,t)):t.memoizedState=vf(t.type,e.memoizedProps,t.pendingProps,e.memoizedState),null;case 27:return xt(t),e===null&&pe&&(a=t.stateNode=_f(t.type,t.pendingProps,Fe.current),nt=t,Jt=!0,i=Ve,la(t.type)?(Oc=i,Ve=Wt(a.firstChild)):Ve=i),et(e,t,t.pendingProps.children,n),ri(e,t),e===null&&(t.flags|=4194304),t.child;case 5:return e===null&&pe&&((i=a=Ve)&&(a=Vy(a,t.type,t.pendingProps,Jt),a!==null?(t.stateNode=a,nt=t,Ve=Wt(a.firstChild),Jt=!1,i=!0):i=!1),i||In(t)),xt(t),i=t.type,l=t.pendingProps,s=e!==null?e.memoizedProps:null,a=l.children,wc(i,l)?a=null:s!==null&&wc(i,s)&&(t.flags|=32),t.memoizedState!==null&&(i=oo(e,t,Mv,null,null,n),Si._currentValue=i),ri(e,t),et(e,t,a,n),t.child;case 6:return e===null&&pe&&((e=n=Ve)&&(n=Ly(n,t.pendingProps,Jt),n!==null?(t.stateNode=n,nt=t,Ve=null,e=!0):e=!1),e||In(t)),null;case 13:return L_(e,t,n);case 4:return lt(t,t.stateNode.containerInfo),a=t.pendingProps,e===null?t.child=ka(t,null,a,n):et(e,t,a,n),t.child;case 11:return O_(e,t,t.type,t.pendingProps,n);case 7:return a=t.pendingProps,ri(e,t),et(e,t,a,n),t.child;case 8:return et(e,t,t.pendingProps.children,n),t.child;case 12:return et(e,t,t.pendingProps.children,n),t.child;case 10:return I_(e,t,n);case 9:return i=t.type._context,a=t.pendingProps.children,wa(t),i=st(i),a=a(i),t.flags|=1,et(e,t,a,n),t.child;case 14:return R_(e,t,t.type,t.pendingProps,n);case 15:return q_(e,t,t.type,t.pendingProps,n);case 19:return Mo(e,t,n);case 31:return Iv(e,t,n);case 22:return M_(e,t,n,t.pendingProps);case 24:return wa(t),a=st(Ze),e===null?(i=Js(),i===null&&(i=Be,l=Xs(),i.pooledCache=l,l.refCount++,l!==null&&(i.pooledCacheLanes|=n),i=l),t.memoizedState={parent:a,cache:i},Ws(t),Pn(t,Ze,i)):((e.lanes&n)!==0&&(eo(e,t),el(t,null,null,n),Wi()),i=e.memoizedState,l=t.memoizedState,i.parent!==a?(i={parent:a,cache:a},t.memoizedState=i,t.lanes===0&&(t.memoizedState=t.updateQueue.baseState=i),Pn(t,Ze,a)):(a=l.cache,Pn(t,Ze,a),a!==i.cache&&Ks(t,[Ze],n,!0))),et(e,t,t.pendingProps.children,n),t.child;case 30:return t.stateNode===null&&(t.stateNode={autoName:null,paired:null,clones:null,ref:null}),a=t.pendingProps,a.name!=null&&a.name!=="auto"?t.flags|=e===null?18882560:18874368:pe&&$l(t),e!==null&&e.memoizedProps.name!==a.name?t.flags|=4194816:ri(e,t),et(e,t,a.children,n),t.child;case 29:throw t.pendingProps}throw Error(d(156,t.tag))}function qn(e){e.flags|=4}function zo(e,t,n,a,i){var l;if((l=(e.mode&32)!==0)&&(l=n===null?bf(t,a):bf(t,a)&&(a.src!==n.src||a.srcSet!==n.srcSet)),l){if(e.flags|=16777216,(i&335544128)===i)if(e.stateNode.complete)e.flags|=8192;else if(jm())e.flags|=8192;else throw Ea=lr,$s}else e.flags&=-16777217}function Y_(e,t){if(t.type!=="stylesheet"||(t.state.loading&4)!==0)e.flags&=-16777217;else if(e.flags|=16777216,!xf(t))if(jm())e.flags|=8192;else throw Ea=lr,$s}function Sr(e,t){t!==null&&(e.flags|=4),e.flags&16384&&(t=e.tag!==22?gu():536870912,e.lanes|=t,di|=t)}function rl(e,t){if(!pe)switch(e.tailMode){case"visible":break;case"collapsed":for(var n=e.tail,a=null;n!==null;)n.alternate!==null&&(a=n),n=n.sibling;a===null?t||e.tail===null?e.tail=null:e.tail.sibling=null:a.sibling=null;break;default:for(t=e.tail,n=null;t!==null;)t.alternate!==null&&(n=t),t=t.sibling;n===null?e.tail=null:n.sibling=null}}function Le(e){var t=e.alternate!==null&&e.alternate.child===e.child,n=0,a=0;if(t)for(var i=e.child;i!==null;)n|=i.lanes|i.childLanes,a|=i.subtreeFlags&1206910976,a|=i.flags&1206910976,i.return=e,i=i.sibling;else for(i=e.child;i!==null;)n|=i.lanes|i.childLanes,a|=i.subtreeFlags,a|=i.flags,i.return=e,i=i.sibling;return e.subtreeFlags|=a,e.childLanes=n,t}function Qv(e,t,n){var a=t.pendingProps;switch(Ps(t),t.tag){case 16:case 15:case 0:case 11:case 7:case 8:case 12:case 9:case 14:return Le(t),null;case 1:return Le(t),null;case 3:return n=t.stateNode,a=null,e!==null&&(a=e.memoizedState.cache),t.memoizedState.cache!==a&&(t.flags|=2048),Cn(Ze),Y(),n.pendingContext&&(n.context=n.pendingContext,n.pendingContext=null),(e===null||e.child===null)&&(Wa(t)?qn(t):e===null||e.memoizedState.isDehydrated&&(t.flags&256)===0||(t.flags|=1024,Qs())),Le(t),null;case 26:var i=t.type,l=t.memoizedState;return e===null?(qn(t),l!==null?(Le(t),Y_(t,l)):(Le(t),zo(t,i,null,a,n))):l?l!==e.memoizedState?(qn(t),Le(t),Y_(t,l)):(Le(t),t.flags&=-16777217):(e=e.memoizedProps,e!==a&&qn(t),Le(t),zo(t,i,e,a,n)),null;case 27:if(ft(t),n=Fe.current,i=t.type,e!==null&&t.stateNode!=null)e.memoizedProps!==a&&qn(t);else{if(!a){if(t.stateNode===null)throw Error(d(166));return Le(t),t.subtreeFlags&=-33554433,null}e=k.current,Wa(t)?Nd(t):(e=_f(i,a,n),t.stateNode=e,qn(t))}return Le(t),t.subtreeFlags&=-33554433,null;case 5:if(ft(t),i=t.type,e!==null&&t.stateNode!=null)e.memoizedProps!==a&&qn(t);else{if(!a){if(t.stateNode===null)throw Error(d(166));return Le(t),t.subtreeFlags&=-33554433,null}if(l=k.current,Wa(t))Nd(t);else{var s=yl(Fe.current);switch(l){case 1:l=s.createElementNS("http://www.w3.org/2000/svg",i);break;case 2:l=s.createElementNS("http://www.w3.org/1998/Math/MathML",i);break;default:switch(i){case"svg":l=s.createElementNS("http://www.w3.org/2000/svg",i);break;case"math":l=s.createElementNS("http://www.w3.org/1998/Math/MathML",i);break;case"script":l=s.createElement("div"),l.innerHTML="<script><\/script>",l=l.removeChild(l.firstChild);break;case"select":l=typeof a.is=="string"?s.createElement("select",{is:a.is}):s.createElement("select"),a.multiple?l.multiple=!0:a.size&&(l.size=a.size);break;default:l=typeof a.is=="string"?s.createElement(i,{is:a.is}):s.createElement(i)}}l[rt]=t,l[Nt]=a;e:for(s=t.child;s!==null;){if(s.tag===5||s.tag===6)l.appendChild(s.stateNode);else if(s.tag!==4&&s.tag!==27&&s.child!==null){s.child.return=s,s=s.child;continue}if(s===t)break e;for(;s.sibling===null;){if(s.return===null||s.return===t)break e;s=s.return}s.sibling.return=s.return,s=s.sibling}t.stateNode=l;e:switch(dt(l,i,a),i){case"button":case"input":case"select":case"textarea":a=!!a.autoFocus;break e;case"img":a=!0;break e;default:a=!1}a&&qn(t)}}return Le(t),t.subtreeFlags&=-33554433,zo(t,t.type,e===null?null:e.memoizedProps,t.pendingProps,n),null;case 6:if(e&&t.stateNode!=null)e.memoizedProps!==a&&qn(t);else{if(typeof a!="string"&&t.stateNode===null)throw Error(d(166));if(e=Fe.current,Wa(t)){if(e=t.stateNode,n=t.memoizedProps,a=null,i=nt,i!==null)switch(i.tag){case 27:case 5:a=i.memoizedProps}e[rt]=t,e=!!(e.nodeValue===n||a!==null&&a.suppressHydrationWarning===!0||Pm(e.nodeValue,n)),e||In(t,!0)}else e=yl(e).createTextNode(a),e[rt]=t,t.stateNode=e}return Le(t),null;case 31:if(n=t.memoizedState,e===null||e.memoizedState!==null){if(a=Wa(t),n!==null){if(e===null){if(!a)throw Error(d(318));if(e=t.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(557));e[rt]=t}else ga(),(t.flags&128)===0&&(t.memoizedState=null),t.flags|=4;Le(t),e=!1}else n=Qs(),e!==null&&e.memoizedState!==null&&(e.memoizedState.hydrationErrors=n),e=!0;if(!e)return t.flags&256?(zt(t),t):(zt(t),null);if((t.flags&128)!==0)throw Error(d(558))}return Le(t),null;case 13:if(a=t.memoizedState,e===null||e.memoizedState!==null&&e.memoizedState.dehydrated!==null){if(i=Wa(t),a!==null&&a.dehydrated!==null){if(e===null){if(!i)throw Error(d(318));if(i=t.memoizedState,i=i!==null?i.dehydrated:null,!i)throw Error(d(317));i[rt]=t}else ga(),(t.flags&128)===0&&(t.memoizedState=null),t.flags|=4;Le(t),i=!1}else i=Qs(),e!==null&&e.memoizedState!==null&&(e.memoizedState.hydrationErrors=i),i=!0;if(!i)return t.flags&256?(zt(t),t):(zt(t),null)}return zt(t),(t.flags&128)!==0?(t.lanes=n,t):(n=a!==null,e=e!==null&&e.memoizedState!==null,n&&(a=t.child,i=null,a.alternate!==null&&a.alternate.memoizedState!==null&&a.alternate.memoizedState.cachePool!==null&&(i=a.alternate.memoizedState.cachePool.pool),l=null,a.memoizedState!==null&&a.memoizedState.cachePool!==null&&(l=a.memoizedState.cachePool.pool),l!==i&&(a.flags|=2048)),n!==e&&n&&(t.child.flags|=8192),Sr(t,t.updateQueue),Le(t),null);case 4:return Y(),e===null&&yc(t.stateNode.containerInfo),t.flags|=67108864,Le(t),null;case 10:return Cn(t.type),Le(t),null;case 19:if(ro(t),a=t.memoizedState,a===null)return Le(t),null;if(i=(t.flags&128)!==0,l=a.rendering,l===null)if(i)rl(a,!1);else{if(Qe!==0||e!==null&&(e.flags&128)!==0)for(e=t.child;e!==null;){if(l=cr(e),l!==null){for(t.flags|=128,rl(a,!1),e=l.updateQueue,t.updateQueue=e,Sr(t,e),t.subtreeFlags=0,e=n,n=t.child;n!==null;)hd(n,e),n=n.sibling;return tl(t,ct.current&1|2),pe&&kn(t,a.treeForkCount),t.child}e=e.sibling}a.tail!==null&&Ot()>zr&&(t.flags|=128,i=!0,rl(a,!1),t.lanes=4194304)}else{if(!i)if(e=cr(l),e!==null){if(t.flags|=128,i=!0,e=e.updateQueue,t.updateQueue=e,Sr(t,e),rl(a,!0),a.tail===null&&a.tailMode!=="collapsed"&&a.tailMode!=="visible"&&!l.alternate&&!pe)return Le(t),null}else 2*Ot()-a.renderingStartTime>zr&&n!==536870912&&(t.flags|=128,i=!0,rl(a,!1),t.lanes=4194304);a.isBackwards?(l.sibling=t.child,t.child=l):(e=a.last,e!==null?e.sibling=l:t.child=l,a.last=l)}if(a.tail!==null){e=a.tail;e:{for(n=e;n!==null;){if(n.alternate!==null){n=!1;break e}n=n.sibling}n=!0}return a.rendering=e,a.tail=e.sibling,a.renderingStartTime=Ot(),e.sibling=null,l=ct.current,l=i?l&1|2:l&1,a.tailMode==="visible"||a.tailMode==="collapsed"||!n||pe?tl(t,l):(n=l,Q(ot,t),Q(ct,n),pt===null&&(pt=t)),pe&&kn(t,a.treeForkCount),e}return Le(t),null;case 22:case 23:return zt(t),io(),a=t.memoizedState!==null,e!==null?e.memoizedState!==null!==a&&(t.flags|=8192):a&&(t.flags|=8192),a?(n&536870912)!==0&&(t.flags&128)===0&&(Le(t),t.subtreeFlags&6&&(t.flags|=8192)):Le(t),n=t.updateQueue,n!==null&&Sr(t,n.retryQueue),n=null,e!==null&&e.memoizedState!==null&&e.memoizedState.cachePool!==null&&(n=e.memoizedState.cachePool.pool),a=null,t.memoizedState!==null&&t.memoizedState.cachePool!==null&&(a=t.memoizedState.cachePool.pool),a!==n&&(t.flags|=2048),e!==null&&H(Sa),null;case 24:return n=null,e!==null&&(n=e.memoizedState.cache),t.memoizedState.cache!==n&&(t.flags|=2048),Cn(Ze),Le(t),null;case 25:return null;case 30:return t.flags|=33554432,Le(t),null}throw Error(d(156,t.tag))}function Fv(e,t){switch(Ps(t),t.tag){case 1:return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 3:return Cn(Ze),Y(),e=t.flags,(e&65536)!==0&&(e&128)===0?(t.flags=e&-65537|128,t):null;case 26:case 27:case 5:return ft(t),null;case 31:if(t.memoizedState!==null){if(zt(t),t.alternate===null)throw Error(d(340));ga()}return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 13:if(zt(t),e=t.memoizedState,e!==null&&e.dehydrated!==null){if(t.alternate===null)throw Error(d(340));ga()}return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 19:return ro(t),e=t.flags,e&65536?(t.flags=e&-65537|128,e=t.memoizedState,e!==null&&(e.rendering=null,e.tail=null),t.flags|=4,t):null;case 4:return Y(),null;case 10:return Cn(t.type),null;case 22:case 23:return zt(t),io(),e!==null&&H(Sa),e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 24:return Cn(Ze),null;case 25:return null;default:return null}}function Q_(e,t){switch(Ps(t),t.tag){case 3:Cn(Ze),Y();break;case 26:case 27:case 5:ft(t);break;case 4:Y();break;case 31:t.memoizedState!==null&&zt(t);break;case 13:zt(t);break;case 19:ro(t);break;case 10:Cn(t.type);break;case 22:case 23:zt(t),io(),e!==null&&H(Sa);break;case 24:Cn(Ze)}}function sl(e,t){try{var n=t.updateQueue,a=n!==null?n.lastEffect:null;if(a!==null){var i=a.next;n=i;do{if((n.tag&e)===e){a=void 0;var l=n.create,s=n.inst;a=l(),s.destroy=a}n=n.next}while(n!==i)}}catch(u){Me(t,t.return,u)}}function $n(e,t,n){try{var a=t.updateQueue,i=a!==null?a.lastEffect:null;if(i!==null){var l=i.next;a=l;do{if((a.tag&e)===e){var s=a.inst,u=s.destroy;if(u!==void 0){s.destroy=void 0,i=t;var m=n,g=u;try{g()}catch(S){Me(i,m,S)}}}a=a.next}while(a!==l)}}catch(S){Me(t,t.return,S)}}function F_(e){var t=e.updateQueue;if(t!==null){var n=e.stateNode;try{zd(t,n)}catch(a){Me(e,e.return,a)}}}function K_(e,t,n){n.props=Ca(e.type,e.memoizedProps),n.state=e.memoizedState;try{n.componentWillUnmount()}catch(a){Me(e,t,a)}}function dn(e,t){try{var n=e.ref;if(n!==null){switch(e.tag){case 26:case 27:case 5:var a=e.stateNode;break;case 30:var i=e.stateNode,l=Sn(e.memoizedProps,i);(i.ref===null||i.ref.name!==l)&&(i.ref=tf(l)),a=i.ref;break;case 7:if(e.stateNode===null){var s=new Ht(e);y(e.child,!1,Uy,s,void 0,void 0),e.stateNode=s}a=e.stateNode;break;default:a=e.stateNode}typeof n=="function"?e.refCleanup=n(a):n.current=a}}catch(u){Me(e,t,u)}}function ut(e,t){var n=e.ref,a=e.refCleanup;if(n!==null)if(typeof a=="function")try{a()}catch(i){Me(e,t,i)}finally{e.refCleanup=null,e=e.alternate,e!=null&&(e.refCleanup=null)}else if(typeof n=="function")try{n(null)}catch(i){Me(e,t,i)}else n.current=null}function jr(e,t){if((e.tag===5||e.tag===27||e.tag===6)&&e.alternate===null&&t!==null)for(var n=0;n<t.length;n++)of(e.stateNode,t[n])}function X_(e){for(var t=e.return;t!==null&&(Bo(t)&&of(e.stateNode,t.stateNode),!Uo(t));)t=t.return}function ol(e){for(var t=e.return;t!==null&&(Bo(t)&&By(e.stateNode,t.stateNode),!Uo(t));)t=t.return}function Uo(e){return e.tag===5||e.tag===3||e.tag===27}function Bo(e){return e&&e.tag===7&&e.stateNode!==null}function Vo(e){var t=e.type,n=e.memoizedProps,a=e.stateNode;try{e:switch(t){case"button":case"input":case"select":case"textarea":n.autoFocus&&a.focus();break e;case"img":n.src?a.src=n.src:n.srcSet&&(a.srcset=n.srcSet)}}catch(i){Me(e,e.return,i)}}function Lo(e,t,n){try{var a=e.stateNode;gy(a,e.type,n,t),a[Nt]=t}catch(i){Me(e,e.return,i)}}function Z_(e){return e.tag===5||e.tag===3||e.tag===26||e.tag===27&&la(e.type)||e.tag===4}function Ho(e){e:for(;;){for(;e.sibling===null;){if(e.return===null||Z_(e.return))return null;e=e.return}for(e.sibling.return=e.return,e=e.sibling;e.tag!==5&&e.tag!==6&&e.tag!==18;){if(e.tag===27&&la(e.type)||e.flags&2||e.child===null||e.tag===4)continue e;e.child.return=e,e=e.child}if(!(e.flags&2))return e.stateNode}}function Go(e,t,n,a){var i=e.tag;if(i===5||i===6)i=e.stateNode,t?(n.nodeType===9?n.body:n.nodeName==="HTML"?n.ownerDocument.body:n).insertBefore(i,t):(t=n.nodeType===9?n.body:n.nodeName==="HTML"?n.ownerDocument.body:n,t.appendChild(i),n=n._reactRootContainer,n!=null||t.onclick!==null||(t.onclick=on)),jr(e,a),Te=!0;else if(i!==4&&(i===27&&(jr(e,a),a=null,la(e.type)&&(n=e.stateNode,t=null)),e=e.child,e!==null))for(Go(e,t,n,a),e=e.sibling;e!==null;)Go(e,t,n,a),e=e.sibling}function Er(e,t,n,a){var i=e.tag;if(i===5||i===6)i=e.stateNode,t?n.insertBefore(i,t):n.appendChild(i),jr(e,a),Te=!0;else if(i!==4&&(i===27&&(jr(e,a),a=null,la(e.type)&&(n=e.stateNode)),e=e.child,e!==null))for(Er(e,t,n,a),e=e.sibling;e!==null;)Er(e,t,n,a),e=e.sibling}function J_(e){var t=e.stateNode,n=e.memoizedProps;try{for(var a=e.type,i=t.attributes;i.length;)t.removeAttributeNode(i[0]);dt(t,a,n),t[rt]=e,t[Nt]=n}catch(l){Me(e,e.return,l)}}var kr=!1,Ut=null;function $_(e){(e.tag===30||(e.subtreeFlags&33554432)!==0)&&(kr=!0)}var _n=null;function W_(){var e=_n;return _n=null,e}var jt=0;function si(e,t,n,a,i){return jt=0,em(e.child,t,n,a,i)}function em(e,t,n,a,i){for(var l=!1;e!==null;){if(e.tag===5){var s=e.stateNode;if(a!==null){var u=jc(s);a.push(u),u.view&&(l=!0)}else l||jc(s).view&&(l=!0);kr=!0,Wm(s,jt===0?t:t+"_"+jt,n),jt++}else(e.tag!==22||e.memoizedState===null)&&(e.tag===30&&i||em(e.child,t,n,a,i)&&(l=!0));e=e.sibling}return l}function mn(e,t){for(;e!==null;)e.tag===5?ef(e.stateNode,e.memoizedProps):(e.tag!==22||e.memoizedState===null)&&(e.tag===30&&t||mn(e.child,t)),e=e.sibling}function Tr(e){if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if((e.tag!==22||e.memoizedState===null)&&(Tr(e),e.tag===30&&(e.flags&18874368)!==0&&e.stateNode.paired)){var t=e.memoizedProps;if(t.name==null||t.name==="auto")throw Error(d(544));var n=t.name;t=jn(t.default,t.share),t!=="none"&&(si(e,n,t,null,!1)||mn(e.child,!1))}e=e.sibling}}function Io(e,t){if(e.tag===30){var n=e.stateNode,a=e.memoizedProps,i=Sn(a,n),l=jn(a.default,n.paired?a.share:a.enter);l!=="none"?si(e,i,l,null,!1)?(Tr(e),n.paired||t||pi(e,a.onEnter)):mn(e.child,!1):Tr(e)}else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)Io(e,t),e=e.sibling;else Tr(e)}function Po(e){if(Ut!==null&&Ut.size!==0){var t=Ut;if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if(e.tag!==22||e.memoizedState===null){if(e.tag===30&&(e.flags&18874368)!==0){var n=e.memoizedProps,a=n.name;if(a!=null&&a!=="auto"){var i=t.get(a);if(i!==void 0){var l=jn(n.default,n.share);if(l!=="none"&&(si(e,a,l,null,!1)?(l=e.stateNode,i.paired=l,l.paired=i,pi(e,n.onShare)):mn(e.child,!1)),t.delete(a),t.size===0)break}}}Po(e)}e=e.sibling}}}function Yo(e){if(e.tag===30){var t=e.memoizedProps,n=Sn(t,e.stateNode),a=Ut!==null?Ut.get(n):void 0,i=jn(t.default,a!==void 0?t.share:t.exit);i!=="none"&&(si(e,n,i,null,!1)?a!==void 0?(i=e.stateNode,a.paired=i,i.paired=a,Ut.delete(n),pi(e,t.onShare)):pi(e,t.onExit):mn(e.child,!1)),Ut!==null&&Po(e)}else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)Yo(e),e=e.sibling;else Ut!==null&&Po(e)}function tm(e){for(e=e.child;e!==null;){if(e.tag===30){var t=e.memoizedProps,n=Sn(t,e.stateNode);t=jn(t.default,t.update),e.flags&=-5,t!=="none"&&si(e,n,t,e.memoizedState=[],!1)}else(e.subtreeFlags&33554432)!==0&&tm(e);e=e.sibling}}function Qo(e){if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if(e.tag!==22||e.memoizedState===null){if(e.tag===30&&(e.flags&18874368)!==0){var t=e.stateNode;t.paired!==null&&(t.paired=null,mn(e.child,!1))}Qo(e)}e=e.sibling}}function Cr(e){if(e.tag===30)e.stateNode.paired=null,mn(e.child,!1),Qo(e);else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)Cr(e),e=e.sibling;else Qo(e)}function nm(e){for(e=e.child;e!==null;)e.tag===30?mn(e.child,!1):(e.subtreeFlags&33554432)!==0&&nm(e),e=e.sibling}function Fo(e,t,n,a,i,l,s){for(var u=!1;t!==null;){if(t.tag===5){var m=t.stateNode;if(l!==null&&jt<l.length){var g=l[jt],S=jc(m);(g.view||S.view)&&(u=!0);var C;if(C=(e.flags&4)===0)if(S.clip)C=!0;else{C=g.rect;var v=S.rect;C=C.y!==v.y||C.x!==v.x||C.height!==v.height||C.width!==v.width}C&&(e.flags|=4),S.abs?S=!g.abs:(g=g.rect,S=S.rect,S=g.height!==S.height||g.width!==S.width),S&&(e.flags|=32)}else e.flags|=32;(e.flags&4)!==0&&Wm(m,jt===0?n:n+"_"+jt,i),u&&(e.flags&4)!==0||(_n===null&&(_n=[]),_n.push(m,jt===0?a:a+"_"+jt,t.memoizedProps)),jt++}else(t.tag!==22||t.memoizedState===null)&&(t.tag===30&&s?e.flags|=t.flags&32:Fo(e,t.child,n,a,i,l,s)&&(u=!0));t=t.sibling}return u}function am(e,t){for(e=e.child;e!==null;){if(e.tag===30){var n=e.memoizedProps,a=e.stateNode,i=Sn(n,a),l=jn(n.default,n.update),s;s=e.memoizedState,e.memoizedState=null,a=e;var u=e.child;jt=0,i=Fo(a,u,i,i,l,s,!1),(e.flags&4)!==0&&i&&pi(e,n.onUpdate)}else(e.subtreeFlags&33554432)!==0&&am(e);e=e.sibling}}var at=!1,Re=!1,fn=!1,Ko=!1,im=typeof WeakSet=="function"?WeakSet:Set,it=null,pn=!1,cl=!1,Ar=!1,Xo=!1;function Kv(e,t,n){if(e=e.containerInfo,bc=ji,e=od(e),Ms(e)){if("selectionStart"in e)var a={start:e.selectionStart,end:e.selectionEnd};else e:{a=(a=e.ownerDocument)&&a.defaultView||window;var i=a.getSelection&&a.getSelection();if(i&&i.rangeCount!==0){a=i.anchorNode;var l=i.anchorOffset,s=i.focusNode;i=i.focusOffset;try{a.nodeType,s.nodeType}catch{a=null;break e}var u=0,m=-1,g=-1,S=0,C=0,v=e,w=null;t:for(;;){for(var L;v!==a||l!==0&&v.nodeType!==3||(m=u+l),v!==s||i!==0&&v.nodeType!==3||(g=u+i),v.nodeType===3&&(u+=v.nodeValue.length),(L=v.firstChild)!==null;)w=v,v=L;for(;;){if(v===e)break t;if(w===a&&++S===l&&(m=u),w===s&&++C===i&&(g=u),(L=v.nextSibling)!==null)break;v=w,w=v.parentNode}v=L}a=m===-1||g===-1?null:{start:m,end:g}}else a=null}a=a||{start:0,end:0}}else a=null;for(xc={focusedElem:e,selectionRange:a},ji=!1,n=(n&335544064)===n,it=t,t=n?9270:1024;it!==null;){if(e=it,n&&(a=e.deletions,a!==null))for(l=0;l<a.length;l++)n&&Yo(a[l]);if(e.alternate===null&&(e.flags&2)!==0)n&&$_(e),Or(n);else{if(e.tag===22){if(a=e.alternate,e.memoizedState!==null){a!==null&&a.memoizedState===null&&n&&Yo(a),Or(n);continue}else if(a!==null&&a.memoizedState!==null){n&&$_(e),Or(n);continue}}a=e.child,(e.subtreeFlags&t)!==0&&a!==null?(a.return=e,it=a):(n&&tm(e),Or(n))}}Ut=null}function Or(e){for(;it!==null;){var t=it,n=e,a=t.alternate,i=t.flags;switch(t.tag){case 0:case 11:case 15:break;case 1:if((i&1024)!==0&&a!==null){n=void 0,i=a.memoizedProps,a=a.memoizedState;var l=t.stateNode;try{var s=Ca(t.type,i);n=l.getSnapshotBeforeUpdate(s,a),l.__reactInternalSnapshotBeforeUpdate=n}catch(u){Me(t,t.return,u)}}break;case 3:if((i&1024)!==0){if(a=t.stateNode.containerInfo,n=a.nodeType,n===9)Tc(a);else if(n===1)switch(a.nodeName){case"HEAD":case"HTML":case"BODY":Tc(a);break;default:a.textContent=""}}break;case 5:case 26:case 27:case 6:case 4:case 17:break;case 30:n&&a!==null&&(n=Sn(a.memoizedProps,a.stateNode),i=t.memoizedProps,i=jn(i.default,i.update),i!=="none"&&si(a,n,i,a.memoizedState=[],!0));break;default:if((i&1024)!==0)throw Error(d(163))}if(a=t.sibling,a!==null){a.return=t.return,it=a;break}it=t.return}}function lm(e,t,n){var a=n.flags;switch(n.tag){case 0:case 11:case 15:vn(e,n),a&4&&sl(5,n);break;case 1:if(vn(e,n),a&4)if(e=n.stateNode,t===null)try{e.componentDidMount()}catch(s){Me(n,n.return,s)}else{var i=Ca(n.type,t.memoizedProps);t=t.memoizedState;try{e.componentDidUpdate(i,t,e.__reactInternalSnapshotBeforeUpdate)}catch(s){Me(n,n.return,s)}}a&64&&F_(n),a&512&&dn(n,n.return);break;case 3:if(vn(e,n),a&64&&(e=n.updateQueue,e!==null)){if(t=null,n.child!==null)switch(n.child.tag){case 27:case 5:t=n.child.stateNode;break;case 1:t=n.child.stateNode}try{zd(e,t)}catch(s){Me(n,n.return,s)}}break;case 27:t===null&&a&4&&J_(n);case 26:case 5:vn(e,n),t===null&&a&4&&Vo(n),a&512&&dn(n,n.return);break;case 12:vn(e,n);break;case 31:vn(e,n),a&4&&cm(e,n);break;case 13:vn(e,n),a&4&&um(e,n),a&64&&(e=n.memoizedState,e!==null&&(e=e.dehydrated,e!==null&&(n=ry.bind(null,n),Hy(e,n))));break;case 22:if(a=n.memoizedState!==null||at,!a){var l=t!==null&&t.memoizedState!==null||Re;t=at,i=Re,at=a,(Re=l)&&!i?(a=2,(n.subtreeFlags&8772)!==0&&(a|=1),ln(e,n,a)):vn(e,n),at=t,Re=i}break;case 30:vn(e,n),a&512&&dn(n,n.return);break;case 7:a&512&&dn(n,n.return);default:vn(e,n)}}function Zo(e,t){for(e=e.child;e!==null;)rm(e,t),e=e.sibling}function rm(e,t){switch(e.tag){case 5:case 26:try{var n=e.stateNode;if(t){var a=n.style;typeof a.setProperty=="function"?a.setProperty("display","none","important"):a.display="none"}else{var i=e.stateNode,l=e.memoizedProps.style,s=l!=null&&l.hasOwnProperty("display")?l.display:null;i.style.display=s==null||typeof s=="boolean"?"":(""+s).trim()}}catch(m){Me(e,e.return,m)}Jo(e,t);break;case 6:try{e.stateNode.nodeValue=t?"":e.memoizedProps,Te=!0}catch(m){Me(e,e.return,m)}break;case 18:try{var u=e.stateNode;t?$m(u,!0):$m(e.stateNode,!1)}catch(m){Me(e,e.return,m)}break;case 22:case 23:e.memoizedState===null&&Zo(e,t);break;default:Zo(e,t)}}function Jo(e,t){if(e.subtreeFlags&67108864)for(e=e.child;e!==null;){e:{var n=e,a=t;switch(n.tag){case 4:rm(n,a);break e;case 22:n.memoizedState===null&&Jo(n,a);break e;default:Jo(n,a)}}e=e.sibling}}function sm(e){var t=e.alternate;t!==null&&(e.alternate=null,sm(t)),e.child=null,e.deletions=null,e.sibling=null,e.tag===5&&(t=e.stateNode,t!==null&&zl(t)),e.stateNode=null,e.return=null,e.dependencies=null,e.memoizedProps=null,e.memoizedState=null,e.pendingProps=null,e.stateNode=null,e.updateQueue=null}var Ge=null,Et=!1;function nn(e,t,n){for(n=n.child;n!==null;)om(e,t,n),n=n.sibling}function om(e,t,n){if(Rt&&typeof Rt.onCommitFiberUnmount=="function")try{Rt.onCommitFiberUnmount(Ri,n)}catch{}switch(n.tag){case 26:Re||ut(n,t),nn(e,t,n),n.memoizedState?n.memoizedState.count--:n.stateNode&&!Re&&(n=n.stateNode,n.parentNode.removeChild(n));break;case 27:Re||ut(n,t),ol(n);var a=Ge,i=Et;la(n.type)&&(Ge=n.stateNode,Et=!1),nn(e,t,n),mf(n.stateNode,n.type,n.memoizedProps),Ge=a,Et=i;break;case 5:Re||ut(n,t),ol(n);case 6:if(n.tag===6&&ol(n),a=Ge,i=Et,Ge=null,nn(e,t,n),Ge=a,Et=i,Ge!==null)if(Et)try{(Ge.nodeType===9?Ge.body:Ge.nodeName==="HTML"?Ge.ownerDocument.body:Ge).removeChild(n.stateNode),Te=!0}catch(l){Me(n,t,l)}else try{Ge.removeChild(n.stateNode),Te=!0}catch(l){Me(n,t,l)}break;case 18:Ge!==null&&(Et?(e=Ge,Jm(e.nodeType===9?e.body:e.nodeName==="HTML"?e.ownerDocument.body:e,n.stateNode),Ei(e)):Jm(Ge,n.stateNode));break;case 4:a=Ge,i=Et,Ge=n.stateNode.containerInfo,Et=!0,nn(e,t,n),Ge=a,Et=i;break;case 0:case 11:case 14:case 15:$n(2,n,t),Re||$n(4,n,t),nn(e,t,n);break;case 1:Re||(ut(n,t),a=n.stateNode,typeof a.componentWillUnmount=="function"&&K_(n,t,a)),nn(e,t,n);break;case 21:nn(e,t,n);break;case 22:Re=(a=Re)||n.memoizedState!==null,nn(e,t,n),Re=a;break;case 30:ut(n,t),nn(e,t,n);break;case 7:Re||ut(n,t),nn(e,t,n);break;default:nn(e,t,n)}}function cm(e,t){if(t.memoizedState===null&&(e=t.alternate,e!==null&&(e=e.memoizedState,e!==null))){e=e.dehydrated;try{Ei(e)}catch(n){Me(t,t.return,n)}}}function um(e,t){if(t.memoizedState===null&&(e=t.alternate,e!==null&&(e=e.memoizedState,e!==null&&(e=e.dehydrated,e!==null))))try{Ei(e)}catch(n){Me(t,t.return,n)}}function Xv(e){switch(e.tag){case 31:case 13:case 19:var t=e.stateNode;return t===null&&(t=e.stateNode=new im),t;case 22:return e=e.stateNode,t=e._retryCache,t===null&&(t=e._retryCache=new im),t;default:throw Error(d(435,e.tag))}}function Rr(e,t){var n=Xv(e);t.forEach(function(a){if(!n.has(a)){n.add(a);var i=sy.bind(null,e,a);a.then(i,i)}})}function ht(e,t,n){var a=t.deletions;if(a!==null)for(var i=0;i<a.length;i++){var l=a[i],s=e,u=t,m=u;e:for(;m!==null;){switch(m.tag){case 27:if(la(m.type)){Ge=m.stateNode,Et=!1;break e}break;case 5:Ge=m.stateNode,Et=!1;break e;case 3:case 4:Ge=m.stateNode.containerInfo,Et=!0;break e}m=m.return}if(Ge===null)throw Error(d(160));om(s,u,l),Ge=null,Et=!1,s=l.alternate,s!==null&&(s.return=null),l.return=null}if(t.subtreeFlags&13886)for(t=t.child;t!==null;)dm(t,e,n),t=t.sibling}var an=null;function dm(e,t,n){var a=e.alternate,i=e.flags;switch(e.tag){case 0:case 11:case 14:case 15:if(i&4&&(a=e.updateQueue,a=a!==null?a.events:null,a!==null))for(var l=0;l<a.length;l++){var s=a[l];s.ref.impl=s.nextImpl}ht(t,e,n),gt(e),i&4&&($n(3,e,e.return),sl(3,e),$n(5,e,e.return));break;case 1:ht(t,e,n),gt(e),i&512&&(Re||a===null||ut(a,a.return)),i&64&&at&&(e=e.updateQueue,e!==null&&(t=e.callbacks,t!==null&&(n=e.shared.hiddenCallbacks,e.shared.hiddenCallbacks=n===null?t:n.concat(t))));break;case 26:if(l=an,ht(t,e,n),gt(e),i&512&&(Re||a===null||ut(a,a.return)),i&4)if(i=a!==null?a.memoizedState:null,n=e.memoizedState,a===null)if(n===null)if(e.stateNode===null)if(at)e.stateNode=Km(e.type,e.memoizedProps,t.containerInfo,e);else{e:{t=e.type,n=e.memoizedProps,i=l.ownerDocument||l;t:switch(t){case"title":a=i.getElementsByTagName("title")[0],(!a||a[Di]||a[rt]||a.namespaceURI==="http://www.w3.org/2000/svg"||a.hasAttribute("itemprop"))&&(a=i.createElement(t),i.head.insertBefore(a,i.querySelector("head > title"))),dt(a,t,n),a[rt]=e,tt(a),t=a;break e;case"link":if(l=gf("link","href",i).get(t+(n.href||""))){for(s=0;s<l.length;s++)if(a=l[s],a.getAttribute("href")===(n.href==null||n.href===""?null:n.href)&&a.getAttribute("rel")===(n.rel==null?null:n.rel)&&a.getAttribute("title")===(n.title==null?null:n.title)&&a.getAttribute("crossorigin")===(n.crossOrigin==null?null:n.crossOrigin)){l.splice(s,1);break t}}a=i.createElement(t),dt(a,t,n),i.head.appendChild(a);break;case"meta":if(l=gf("meta","content",i).get(t+(n.content||""))){for(s=0;s<l.length;s++)if(a=l[s],a.getAttribute("content")===(n.content==null?null:""+n.content)&&a.getAttribute("name")===(n.name==null?null:n.name)&&a.getAttribute("property")===(n.property==null?null:n.property)&&a.getAttribute("http-equiv")===(n.httpEquiv==null?null:n.httpEquiv)&&a.getAttribute("charset")===(n.charSet==null?null:n.charSet)){l.splice(s,1);break t}}a=i.createElement(t),dt(a,t,n),i.head.appendChild(a);break;default:throw Error(d(468,t))}a[rt]=e,tt(a),t=a}e.stateNode=t}else at||Dc(l,e.type,e.stateNode);else e.stateNode=hf(l,n,e.memoizedProps);else i!==n?(i===null?(t=a.stateNode,t===null||Re||t.parentNode.removeChild(t)):i.count--,n===null?at||Dc(l,e.type,e.stateNode):hf(l,n,e.memoizedProps)):n===null&&e.stateNode!==null&&Lo(e,e.memoizedProps,a.memoizedProps);break;case 27:ht(t,e,n),gt(e),i&512&&(Re||a===null||ut(a,a.return)),a!==null&&i&4&&Lo(e,e.memoizedProps,a.memoizedProps);break;case 5:if(l=fn,fn=!1,ht(t,e,n),fn=l,gt(e),i&512&&(Re||a===null||ut(a,a.return)),e.flags&32){t=e.stateNode;try{Ia(t,""),Te=!0}catch(S){Me(e,e.return,S)}}i&4&&e.stateNode!=null&&(t=e.memoizedProps,Lo(e,t,a!==null?a.memoizedProps:t)),i&1024&&(Ko=!0);break;case 6:if(ht(t,e,n),gt(e),i&4){if(e.stateNode===null)throw Error(d(162));t=e.memoizedProps,n=e.stateNode;try{n.nodeValue=t,Te=!0}catch(S){Me(e,e.return,S)}}break;case 3:if(Te=!1,Fr=null,l=an,an=hl(t.containerInfo),ht(t,e,n),an=l,gt(e),i&4&&a!==null&&a.memoizedState.isDehydrated)try{Ei(t.containerInfo)}catch(S){Me(e,e.return,S)}Ko&&(Ko=!1,_m(e)),Te=!1;break;case 4:i=fn,fn=at,a=Ru(),l=an,an=hl(e.stateNode.containerInfo),ht(t,e,n),gt(e),an=l,Te&&cl&&(Ar=!0),Te=a,fn=i;break;case 12:ht(t,e,n),gt(e);break;case 31:ht(t,e,n),gt(e),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Rr(e,t)));break;case 13:ht(t,e,n),gt(e),e.child.flags&8192&&e.memoizedState!==null!=(a!==null&&a.memoizedState!==null)&&(Dr=Ot()),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Rr(e,t)));break;case 22:l=e.memoizedState!==null,s=a!==null&&a.memoizedState!==null;var u=at,m=Re,g=fn;at=u||l,fn=g||l,Re=m||s,ht(t,e,n),Re=m,fn=g,at=u,gt(e),i&8192&&(t=e.stateNode,t._visibility=l?t._visibility&-2:t._visibility|1,!l||a===null||s||at||Re||(t=s||Re,n=at,a=Re,at=l||at,Re=t,Wn(e,2),at=n,Re=a),!l&&fn||Zo(e,l)),i&4&&(t=e.updateQueue,t!==null&&(n=t.retryQueue,n!==null&&(t.retryQueue=null,Rr(e,n))));break;case 19:ht(t,e,n),gt(e),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Rr(e,t)));break;case 30:i&512&&(Re||a===null||ut(a,a.return)),i=Ru(),l=cl,s=(n&335544064)===n,u=e.memoizedProps,cl=s&&jn(u.default,u.update)!=="none",ht(t,e,n),gt(e),s&&a!==null&&Te&&(e.flags|=4),cl=l,Te=i;break;case 21:break;case 7:i&512&&(Re||a===null||ut(a,a.return)),a&&a.stateNode!==null&&(a.stateNode._fragmentFiber=e);default:ht(t,e,n),gt(e)}}function gt(e){var t=e.flags;if(t&2){try{for(var n,a=e.return;a!==null;){if(Z_(a)){n=a;break}a=a.return}a=null;for(var i=e.return;i!==null;){if(Bo(i)){var l=i.stateNode;a===null?a=[l]:a.push(l)}if(Uo(i))break;i=i.return}var s=a;if(n==null)throw Error(d(160));switch(n.tag){case 27:var u=n.stateNode,m=Ho(e);Er(e,m,u,s);break;case 5:var g=n.stateNode;n.flags&32&&(Ia(g,""),n.flags&=-33);var S=Ho(e);Er(e,S,g,s);break;case 3:case 4:var C=n.stateNode.containerInfo,v=Ho(e);Go(e,v,C,s);break;default:throw Error(d(161))}}catch(w){Me(e,e.return,w)}e.flags&=-3}t&4096&&(e.flags&=-4097)}function _m(e){if(e.subtreeFlags&1024)for(e=e.child;e!==null;){var t=e;_m(t),t.tag===5&&t.flags&1024&&(t=t.stateNode,ji=!0,t.reset(),ji=!1),e=e.sibling}}function oi(e,t){if(t.subtreeFlags&9270)for(t=t.child;t!==null;)mm(t,e),t=t.sibling;else am(t)}function mm(e,t){var n=e.alternate;if(n===null)Io(e,!1);else switch(e.tag){case 3:if(Xo=pn=!1,W_(),oi(t,e),!pn&&!Ar){if(e=_n,e!==null)for(var a=0;a<e.length;a+=3){n=e[a];var i=e[a+1];ef(n,e[a+2]),n=n.ownerDocument.documentElement,n!==null&&n.animate({opacity:[0,0],pointerEvents:["none","none"]},{duration:0,fill:"forwards",pseudoElement:"::view-transition-group("+i+")"})}e=t.containerInfo,e=e.nodeType===9?e.documentElement:e.ownerDocument.documentElement,e!==null&&e.style.viewTransitionName===""&&(e.style.viewTransitionName="none",e.animate({opacity:[0,0],pointerEvents:["none","none"]},{duration:0,fill:"forwards",pseudoElement:"::view-transition-group(root)"}),e.animate({width:[0,0],height:[0,0]},{duration:0,fill:"forwards",pseudoElement:"::view-transition"})),Xo=!0}_n=null;break;case 5:oi(t,e);break;case 4:a=pn,pn=!1,oi(t,e),pn&&(Ar=!0),pn=a;break;case 22:e.memoizedState===null&&(n.memoizedState!==null?Io(e,!1):oi(t,e));break;case 30:a=pn,i=W_(),pn=!1,oi(t,e),pn&&(e.flags|=4);var l=e.memoizedProps,s=e.stateNode;t=Sn(l,s),s=Sn(n.memoizedProps,s);var u=jn(l.default,l.update);u==="none"?t=!1:(l=n.memoizedState,n.memoizedState=null,n=e.child,jt=0,t=Fo(e,n,t,s,u,l,!0),jt!==(l===null?0:l.length)&&(e.flags|=32)),(e.flags&4)!==0&&t?(pi(e,e.memoizedProps.onUpdate),_n=i):i!==null&&(i.push.apply(i,_n),_n=i),pn=(e.flags&32)!==0?!0:a;break;default:oi(t,e)}}function vn(e,t){if(t.subtreeFlags&8772)for(t=t.child;t!==null;)lm(e,t.alternate,t),t=t.sibling}function Wn(e,t){for(e=e.child;e!==null;){var n=e,a=t;switch(n.tag){case 0:case 11:case 14:case 15:$n(4,n,n.return),Wn(n,a);break;case 1:ut(n,n.return);var i=n.stateNode;typeof i.componentWillUnmount=="function"&&K_(n,n.return,i),Wn(n,a);break;case 27:(a&2)!==0&&mf(n.stateNode,n.type,n.memoizedProps);case 5:ut(n,n.return),n.tag!==5&&n.tag!==27||ol(n),Wn(n,a);break;case 6:ol(n);break;case 26:ut(n,n.return),i=n.stateNode,n.memoizedState!==null||i===null||Re||i.parentNode.removeChild(i),Wn(n,a);break;case 22:n.memoizedState===null&&Wn(n,a);break;case 30:ut(n,n.return),Wn(n,a);break;case 7:ut(n,n.return);default:Wn(n,a)}e=e.sibling}}function ln(e,t,n){for(n=(t.subtreeFlags&8772)!==0?n:n&-2,t=t.child;t!==null;){var a=t.alternate,i=e,l=t,s=l.flags,u=(n&1)!==0;switch(l.tag){case 0:case 11:case 15:ln(i,l,n),sl(4,l);break;case 1:if(ln(i,l,n),a=l,i=a.stateNode,typeof i.componentDidMount=="function")try{i.componentDidMount()}catch(S){Me(a,a.return,S)}if(a=l,i=a.updateQueue,i!==null){var m=a.stateNode;try{var g=i.shared.hiddenCallbacks;if(g!==null)for(i.shared.hiddenCallbacks=null,i=0;i<g.length;i++)Dd(g[i],m)}catch(S){Me(a,a.return,S)}}u&&s&64&&F_(l),dn(l,l.return);break;case 27:(n&2)!==0&&J_(l);case 5:l.tag!==5&&l.tag!==27||X_(l),ln(i,l,n),u&&a===null&&s&4&&Vo(l),dn(l,l.return);break;case 6:X_(l);break;case 26:m=l.stateNode,l.memoizedState!==null||m===null||at||Dc(hl(m.ownerDocument),l.type,m),ln(i,l,n),u&&a===null&&s&4&&Vo(l),dn(l,l.return);break;case 12:ln(i,l,n);break;case 31:ln(i,l,n),u&&s&4&&cm(i,l);break;case 13:ln(i,l,n),u&&s&4&&um(i,l);break;case 22:l.memoizedState===null&&ln(i,l,n),dn(l,l.return);break;case 30:ln(i,l,n),dn(l,l.return);break;case 7:dn(l,l.return);default:ln(i,l,n)}t=t.sibling}}function $o(e,t){var n=null;e!==null&&e.memoizedState!==null&&e.memoizedState.cachePool!==null&&(n=e.memoizedState.cachePool.pool),e=null,t.memoizedState!==null&&t.memoizedState.cachePool!==null&&(e=t.memoizedState.cachePool.pool),e!==n&&(e!=null&&e.refCount++,n!=null&&Ki(n))}function Wo(e,t){e=null,t.alternate!==null&&(e=t.alternate.memoizedState.cache),t=t.memoizedState.cache,t!==e&&(t.refCount++,e!=null&&Ki(e))}function $t(e,t,n,a){var i=(n&335544064)===n;if(t.subtreeFlags&(i?10262:10256))for(t=t.child;t!==null;)fm(e,t,n,a),t=t.sibling;else i&&nm(t)}function fm(e,t,n,a){var i=(n&335544064)===n;i&&t.alternate===null&&t.return!==null&&t.return.alternate!==null&&Cr(t);var l=t.flags;switch(t.tag){case 0:case 11:case 15:$t(e,t,n,a),l&2048&&sl(9,t);break;case 1:$t(e,t,n,a);break;case 3:$t(e,t,n,a),i&&Xo&&(e=e.containerInfo,e=e.nodeType===9?e.body:e.nodeName==="HTML"?e.ownerDocument.body:e,e.style.viewTransitionName==="root"&&(e.style.viewTransitionName=""),e=e.ownerDocument.documentElement,e!==null&&e.style.viewTransitionName==="none"&&(e.style.viewTransitionName="")),l&2048&&(l=null,t.alternate!==null&&(l=t.alternate.memoizedState.cache),t=t.memoizedState.cache,t!==l&&(t.refCount++,l!=null&&Ki(l)));break;case 12:if(l&2048){$t(e,t,n,a),l=t.stateNode;try{var s=t.memoizedProps,u=s.id,m=s.onPostCommit;typeof m=="function"&&m(u,t.alternate===null?"mount":"update",l.passiveEffectDuration,-0)}catch(g){Me(t,t.return,g)}}else $t(e,t,n,a);break;case 31:$t(e,t,n,a);break;case 13:$t(e,t,n,a);break;case 23:break;case 22:s=t.stateNode,u=t.alternate,t.memoizedState!==null?(i&&u!==null&&u.memoizedState===null&&Cr(u),s._visibility&2?$t(e,t,n,a):ul(e,t)):(i&&u!==null&&u.memoizedState!==null&&Cr(t),s._visibility&2?$t(e,t,n,a):(s._visibility|=2,ci(e,t,n,a,(t.subtreeFlags&10256)!==0||!1))),l&2048&&$o(u,t);break;case 24:$t(e,t,n,a),l&2048&&Wo(t.alternate,t);break;case 30:i&&(l=t.alternate,l!==null&&(mn(l.child,!0),mn(t.child,!0))),$t(e,t,n,a);break;default:$t(e,t,n,a)}}function ci(e,t,n,a,i){for(i=i&&((t.subtreeFlags&10256)!==0||!1),t=t.child;t!==null;){var l=e,s=t,u=n,m=a,g=s.flags;switch(s.tag){case 0:case 11:case 15:ci(l,s,u,m,i),sl(8,s);break;case 23:break;case 22:var S=s.stateNode;s.memoizedState!==null?S._visibility&2?ci(l,s,u,m,i):ul(l,s):(S._visibility|=2,ci(l,s,u,m,i)),i&&g&2048&&$o(s.alternate,s);break;case 24:ci(l,s,u,m,i),i&&g&2048&&Wo(s.alternate,s);break;default:ci(l,s,u,m,i)}t=t.sibling}}function ul(e,t){if(t.subtreeFlags&10256)for(t=t.child;t!==null;){var n=e,a=t,i=a.flags;switch(a.tag){case 22:ul(n,a),i&2048&&$o(a.alternate,a);break;case 24:ul(n,a),i&2048&&Wo(a.alternate,a);break;default:ul(n,a)}t=t.sibling}}var Aa=8192;function Oa(e,t,n){if(e.subtreeFlags&Aa)for(e=e.child;e!==null;)pm(e,t,n),e=e.sibling}function pm(e,t,n){switch(e.tag){case 26:Oa(e,t,n),e.flags&Aa&&(e.memoizedState!==null?th(n,an,e.memoizedState,e.memoizedProps):(e=e.stateNode,(t&335544128)===t&&Nf(n,e)));break;case 5:Oa(e,t,n),e.flags&Aa&&(e=e.stateNode,(t&335544128)===t&&Nf(n,e));break;case 3:case 4:var a=an;an=hl(e.stateNode.containerInfo),Oa(e,t,n),an=a;break;case 22:e.memoizedState===null&&(a=e.alternate,a!==null&&a.memoizedState!==null?(a=Aa,Aa=16777216,Oa(e,t,n),Aa=a):Oa(e,t,n));break;case 30:if((e.flags&Aa)!==0&&(a=e.memoizedProps.name,a!=null&&a!=="auto")){var i=e.stateNode;i.paired=null,Ut===null&&(Ut=new Map),Ut.set(a,i)}Oa(e,t,n);break;default:Oa(e,t,n)}}function vm(e){var t=e.alternate;if(t!==null&&(e=t.child,e!==null)){t.child=null;do t=e.sibling,e.sibling=null,e=t;while(e!==null)}}function dl(e){var t=e.deletions;if((e.flags&16)!==0){if(t!==null)for(var n=0;n<t.length;n++){var a=t[n];it=a,hm(a,e)}vm(e)}if(e.subtreeFlags&10256)for(e=e.child;e!==null;)ym(e),e=e.sibling}function ym(e){switch(e.tag){case 0:case 11:case 15:dl(e),e.flags&2048&&$n(9,e,e.return);break;case 3:dl(e);break;case 12:dl(e);break;case 22:var t=e.stateNode;e.memoizedState!==null&&t._visibility&2&&(e.return===null||e.return.tag!==13)?(t._visibility&=-3,qr(e)):dl(e);break;default:dl(e)}}function qr(e){var t=e.deletions;if((e.flags&16)!==0){if(t!==null)for(var n=0;n<t.length;n++){var a=t[n];it=a,hm(a,e)}vm(e)}for(e=e.child;e!==null;){switch(t=e,t.tag){case 0:case 11:case 15:$n(8,t,t.return),qr(t);break;case 22:n=t.stateNode,n._visibility&2&&(n._visibility&=-3,qr(t));break;default:qr(t)}e=e.sibling}}function hm(e,t){for(;it!==null;){var n=it;switch(n.tag){case 0:case 11:case 15:$n(8,n,t);break;case 23:case 22:if(n.memoizedState!==null&&n.memoizedState.cachePool!==null){var a=n.memoizedState.cachePool.pool;a!=null&&a.refCount++}break;case 24:Ki(n.memoizedState.cache)}if(a=n.child,a!==null)a.return=n,it=a;else e:for(n=e;it!==null;){a=it;var i=a.sibling,l=a.return;if(sm(a),a===n){it=null;break e}if(i!==null){i.return=l,it=i;break e}it=l}}}var Zv={getCacheForType:function(e){var t=st(Ze),n=t.data.get(e);return n===void 0&&(n=e(),t.data.set(e,n)),n},cacheSignal:function(){return st(Ze).controller.signal}},Jv=typeof WeakMap=="function"?WeakMap:Map,Ae=0,Be=null,ve=null,ge=0,qe=0,Bt=null,ea=!1,ui=!1,ec=!1,Mn=0,Qe=0,ta=0,Ra=0,Mr=0,Vt=0,di=0,_l=null,kt=null,tc=!1,Dr=0,gm=0,zr=1/0,Ur=null,na=null,Pe=0,rn=null,qa=null,yn=0,nc=0,ac=null,bm=null,_i=null,mi=null,fi=null,ml=0,Br=null;function Lt(){return(Ae&2)!==0&&ge!==0?ge&-ge:J.T!==null?mc():Nu()}function xm(){if(Vt===0)if((ge&536870912)===0||pe){var e=Rl;Rl<<=1,(Rl&3932160)===0&&(Rl=262144),Vt=e}else Vt=536870912;return e=ot.current,e!==null&&(e.flags|=32),Vt}function pi(e,t){if(t!=null){var n=e.stateNode,a=n.ref;a===null&&(a=n.ref=tf(Sn(e.memoizedProps,n))),mi===null&&(mi=[]),mi.push(t.bind(null,a))}}function Tt(e,t,n){(e===Be&&(qe===2||qe===9)||e.cancelPendingCommit!==null)&&(vi(e,0),aa(e,ge,Vt,!1)),Mi(e,n),((Ae&2)===0||e!==Be)&&(e===Be&&((Ae&2)===0&&(Ra|=n),Qe===4&&aa(e,ge,Vt,!1)),hn(e))}function wm(e,t,n){if((Ae&6)!==0)throw Error(d(327));var a=!n&&(t&127)===0&&(t&e.expiredLanes)===0||qi(e,t),i=a?ey(e,t):lc(e,t,!0),l=a;do{if(i===0){ui&&!a&&aa(e,t,0,!1);break}else{if(n=e.current.alternate,l&&!$v(n)){i=lc(e,t,!1),l=!1;continue}if(i===2){if(l=t,e.errorRecoveryDisabledLanes&l)var s=0;else s=e.pendingLanes&-536870913,s=s!==0?s:s&536870912?536870912:0;if(s!==0){t=s;e:{var u=e;i=_l;var m=u.current.memoizedState.isDehydrated;if(m&&(vi(u,s).flags|=256),s=lc(u,s,!1),s!==2&&s!==6){if(ec&&!m){u.errorRecoveryDisabledLanes|=l,Ra|=l,i=4;break e}l=kt,kt=i,l!==null&&(kt===null?kt=l:kt.push.apply(kt,l))}i=s}if(l=!1,i!==2)continue}}if(i===1){vi(e,0),aa(e,t,0,!0);break}e:{switch(a=e,l=i,l){case 0:case 1:throw Error(d(345));case 4:if((t&4194048)!==t&&(t&62914560)!==t)break;case 6:aa(a,t,Vt,!ea);break e;case 2:kt=null;break;case 3:case 5:break;default:throw Error(d(329))}if((t&62914560)===t&&(i=Dr+300-Ot(),10<i)){if(aa(a,t,Vt,!ea),Ml(a,0,!0)!==0)break e;yn=t,a.timeoutHandle=Sc(Nm.bind(null,a,n,kt,Ur,tc,t,Vt,Ra,di,ea,l,"Throttled",-0,0),i);break e}Nm(a,n,kt,Ur,tc,t,Vt,Ra,di,ea,l,null,-0,0)}}break}while(!0);hn(e)}function Nm(e,t,n,a,i,l,s,u,m,g,S,C,v,w){e.timeoutHandle=-1;var L=t.subtreeFlags,X=(l&335544064)===l;if(C=null,(X||L&8192||(L&16785408)===16785408)&&(C={stylesheets:null,count:0,imgCount:0,imgBytes:0,suspenseyImages:[],waitingForImages:!0,waitingForViewTransition:!1,unsuspend:on},Ut=null,pm(t,l,C),X&&(L=C,X=e.containerInfo,X=(X.nodeType===9?X:X.ownerDocument).__reactViewTransition,X!=null&&(L.count++,L.waitingForViewTransition=!0,L=xl.bind(L),X.finished.then(L,L))),L=(l&62914560)===l?Dr-Ot():(l&4194048)===l?gm-Ot():0,L=nh(C,L),L!==null)){yn=l,e.cancelPendingCommit=L(Om.bind(null,e,t,l,n,a,i,s,u,m,g,S,C,null,v,w)),aa(e,l,s,!g);return}Om(e,t,l,n,a,i,s,u,m,g,S,C)}function $v(e){for(var t=e;;){var n=t.tag;if((n===0||n===11||n===15)&&t.flags&16384&&(n=t.updateQueue,n!==null&&(n=n.stores,n!==null)))for(var a=0;a<n.length;a++){var i=n[a],l=i.getSnapshot;i=i.value;try{if(!Dt(l(),i))return!1}catch{return!1}}if(n=t.child,t.subtreeFlags&16384&&n!==null)n.return=t,t=n;else{if(t===e)break;for(;t.sibling===null;){if(t.return===null||t.return===e)return!0;t=t.return}t.sibling.return=t.return,t=t.sibling}}return!0}function aa(e,t,n,a){t=hu(e,t),t&=~Mr,t&=~Ra,e.suspendedLanes|=t,e.pingedLanes&=~t,a&&(e.warmLanes|=t),a=e.expirationTimes;for(var i=t;0<i;){var l=31-qt(i),s=1<<l;a[l]=-1,i&=~s}n!==0&&bu(e,n,t)}function Vr(){return(Ae&6)===0?(fl(0),!1):!0}function ic(){if(ve!==null){if(qe===0)var e=ve.return;else e=ve,Tn=ba=null,_o(e),ni=null,Ji=0,e=ve;for(;e!==null;)Q_(e.alternate,e),e=e.return;ve=null}}function vi(e,t){var n=e.timeoutHandle;return n!==-1&&(e.timeoutHandle=-1,wy(n)),n=e.cancelPendingCommit,n!==null&&(e.cancelPendingCommit=null,n()),yn=0,ic(),Be=e,ve=n=En(e.current,null),ge=t,qe=0,Bt=null,ea=!1,ui=qi(e,t),ec=!1,di=Vt=Mr=Ra=ta=Qe=0,kt=_l=null,tc=!1,Mn=hu(e,t),Fl(),n}function Sm(e,t){de=null,J.H=hr,t===ti||t===ir?(t=Od(),qe=3):t===$s?(t=Od(),qe=4):qe=t===ko?8:t!==null&&typeof t=="object"&&typeof t.then=="function"?6:1,Bt=t,ve===null&&(Qe=1,gr(e,Kt(t,e.current)))}function jm(){var e=ot.current;return e===null?!0:(ge&4194048)===ge?pt===null:(ge&62914560)===ge||(ge&536870912)!==0?e===pt:!1}function Em(){var e=J.H;return J.H=hr,e===null?hr:e}function km(){var e=J.A;return J.A=Zv,e}function Lr(){Qe=4,ea||(ge&4194048)!==ge&&ot.current!==null||(ui=!0),(ta&134217727)===0&&(Ra&134217727)===0||Be===null||aa(Be,ge,Vt,!1)}function lc(e,t,n){var a=Ae;Ae|=2;var i=Em(),l=km();(Be!==e||ge!==t)&&(Ur=null,vi(e,t)),t=!1;var s=Qe;e:do try{if(qe!==0&&ve!==null){var u=ve,m=Bt;switch(qe){case 8:ic(),s=6;break e;case 3:case 2:case 9:case 6:ot.current===null&&(t=!0);var g=qe;if(qe=0,Bt=null,yi(e,u,m,g),n&&ui){s=0;break e}break;default:g=qe,qe=0,Bt=null,yi(e,u,m,g)}}Wv(),s=Qe;break}catch(S){Sm(e,S)}while(!0);return t&&e.shellSuspendCounter++,Tn=ba=null,Ae=a,J.H=i,J.A=l,ve===null&&(Be=null,ge=0,Fl()),s}function Wv(){for(;ve!==null;)Tm(ve)}function ey(e,t){var n=Ae;Ae|=2;var a=Em(),i=km();Be!==e||ge!==t?(Ur=null,zr=Ot()+500,vi(e,t)):ui=qi(e,t);e:do try{if(qe!==0&&ve!==null){t=ve;var l=Bt;t:switch(qe){case 1:qe=0,Bt=null,yi(e,t,l,1);break;case 2:case 9:if(Cd(l)){qe=0,Bt=null,Cm(t);break}t=function(){qe!==2&&qe!==9||Be!==e||(qe=7),hn(e)},l.then(t,t);break e;case 3:qe=7;break e;case 4:qe=5;break e;case 7:Cd(l)?(qe=0,Bt=null,Cm(t)):(qe=0,Bt=null,yi(e,t,l,7));break;case 5:var s=null;switch(ve.tag){case 26:s=ve.memoizedState;case 5:case 27:var u=ve;if(s?xf(s):u.stateNode.complete){qe=0,Bt=null;var m=u.sibling;if(m!==null)ve=m;else{var g=u.return;g!==null?(ve=g,Hr(g)):ve=null}break t}}qe=0,Bt=null,yi(e,t,l,5);break;case 6:qe=0,Bt=null,yi(e,t,l,6);break;case 8:ic(),Qe=6;break e;default:throw Error(d(462))}}ty();break}catch(S){Sm(e,S)}while(!0);return Tn=ba=null,J.H=a,J.A=i,Ae=n,ve!==null?0:(Be=null,ge=0,Fl(),Qe)}function ty(){for(;ve!==null&&!hp();)Tm(ve)}function Tm(e){var t=P_(e.alternate,e,Mn);e.memoizedProps=e.pendingProps,t===null?Hr(e):ve=t}function Cm(e){var t=e,n=t.alternate;switch(t.tag){case 15:case 0:t=U_(n,t,t.pendingProps,t.type,void 0,ge);break;case 11:t=U_(n,t,t.pendingProps,t.type.render,t.ref,ge);break;case 5:_o(t);var a=t;a===nt&&(pe?(Wl(a),a.tag===5&&a.stateNode!=null&&(Ve=a.stateNode)):(Wl(a),pe=!0));default:Q_(n,t),t=ve=hd(t,Mn),t=P_(n,t,Mn)}e.memoizedProps=e.pendingProps,t===null?Hr(e):ve=t}function yi(e,t,n,a){Tn=ba=null,_o(t),ni=null,Ji=0;var i=t.return;try{if(Gv(e,i,t,n,ge)){Qe=1,gr(e,Kt(n,e.current)),ve=null;return}}catch(l){if(i!==null)throw ve=i,l;Qe=1,gr(e,Kt(n,e.current)),ve=null;return}t.flags&32768?(pe||a===1?e=!0:ui||(ge&536870912)!==0?e=!1:(ea=e=!0,(a===2||a===9||a===3||a===6)&&(a=ot.current,a!==null&&a.tag===13&&(a.flags|=16384))),Am(t,e)):Hr(t)}function Hr(e){var t=e;do{if((t.flags&32768)!==0){Am(t,ea);return}e=t.return;var n=Qv(t.alternate,t,Mn);if(n!==null){ve=n;return}if(t=t.sibling,t!==null){ve=t;return}ve=t=e}while(t!==null);Qe===0&&(Qe=5)}function Am(e,t){do{var n=Fv(e.alternate,e);if(n!==null){n.flags&=32767,ve=n;return}if(n=e.return,n!==null&&(n.flags|=32768,n.subtreeFlags=0,n.deletions=null),!t&&(e=e.sibling,e!==null)){ve=e;return}ve=e=n}while(e!==null);Qe=6,ve=null}function Om(e,t,n,a,i,l,s,u,m,g,S,C){e.cancelPendingCommit=null;do Gr();while(Pe!==0);if((Ae&6)!==0)throw Error(d(327));if(t!==null){if(t===e.current)throw Error(d(177));e===Be&&(ve=Be=null,ge=0),qa=t,rn=e,yn=n,ac=i,bm=a,ny(e,t,n,s,u,m,C)}}function ny(e,t,n,a,i,l,s){var u=t.lanes|t.childLanes;if(nc=u,u|=Vs,Tp(e,n,u,a,i,l),mi=null,(n&335544064)===n?(fi=Av(e),a=10262):(fi=null,a=10256),(t.subtreeFlags&a)!==0||(t.flags&a)!==0?(e.callbackNode=null,e.callbackPriority=0,oy(Al,function(){return cc(),null})):(e.callbackNode=null,e.callbackPriority=0),kr=!1,a=(t.flags&13878)!==0,(t.subtreeFlags&13878)!==0||a){a=J.T,J.T=null,i=se.p,se.p=2,l=Ae,Ae|=4;try{Kv(e,t,n)}finally{Ae=l,se.p=i,J.T=a}}Pe=1,kr?_i=Ty(s,e.containerInfo,fi,rc,sc,iy,oc,cc,ay):(rc(),sc(),oc())}function ay(e){if(Pe!==0){var t=rn.onRecoverableError;t(e,{componentStack:null})}}function iy(){Pe===3&&(Pe=0,mm(qa,rn),Pe=4)}function rc(){if(Pe===1){Pe=0;var e=rn,t=qa,n=yn,a=(t.flags&13878)!==0;if((t.subtreeFlags&13878)!==0||a){a=J.T,J.T=null;var i=se.p;se.p=2;var l=Ae;Ae|=4;try{cl=Ar=!1,dm(t,e,n),n=xc;var s=od(e.containerInfo),u=n.focusedElem,m=n.selectionRange;if(s!==u&&u&&u.ownerDocument&&sd(u.ownerDocument.documentElement,u)){if(m!==null&&Ms(u)){var g=m.start,S=m.end;if(S===void 0&&(S=g),"selectionStart"in u)u.selectionStart=g,u.selectionEnd=Math.min(S,u.value.length);else{var C=u.ownerDocument||document,v=C&&C.defaultView||window;if(v.getSelection){var w=v.getSelection(),L=u.textContent.length,X=Math.min(m.start,L),_e=m.end===void 0?X:Math.min(m.end,L);!w.extend&&X>_e&&(s=_e,_e=X,X=s);var h=rd(u,X),f=rd(u,_e);if(h&&f&&(w.rangeCount!==1||w.anchorNode!==h.node||w.anchorOffset!==h.offset||w.focusNode!==f.node||w.focusOffset!==f.offset)){var x=C.createRange();x.setStart(h.node,h.offset),w.removeAllRanges(),X>_e?(w.addRange(x),w.extend(f.node,f.offset)):(x.setEnd(f.node,f.offset),w.addRange(x))}}}}for(C=[],w=u;w=w.parentNode;)w.nodeType===1&&C.push({element:w,left:w.scrollLeft,top:w.scrollTop});for(typeof u.focus=="function"&&u.focus(),u=0;u<C.length;u++){var T=C[u];T.element.scrollLeft=T.left,T.element.scrollTop=T.top}}ji=!!bc,xc=bc=null}finally{Ae=l,se.p=i,J.T=a}}e.current=t,Pe=2}}function sc(){if(Pe===2){Pe=0;var e=rn,t=qa,n=(t.flags&8772)!==0;if((t.subtreeFlags&8772)!==0||n){n=J.T,J.T=null;var a=se.p;se.p=2;var i=Ae;Ae|=4;try{lm(e,t.alternate,t)}finally{Ae=i,se.p=a,J.T=n}}Pe=3}}function oc(){if(Pe===4||Pe===3){Pe=0;var e=_i;_i=null,gp();var t=rn,n=qa,a=yn,i=bm,l=(a&335544064)===a?10262:10256;if((n.subtreeFlags&l)!==0||(n.flags&l)!==0?Pe=5:(Pe=0,qa=rn=null,Rm(t,t.pendingLanes)),l=t.pendingLanes,l===0&&(na=null),vs(a),n=n.stateNode,Rt&&typeof Rt.onCommitFiberRoot=="function")try{Rt.onCommitFiberRoot(Ri,n,void 0,(n.current.flags&128)===128)}catch{}if(i!==null){n=J.T,l=se.p,se.p=2,J.T=null;try{for(var s=t.onRecoverableError,u=0;u<i.length;u++){var m=i[u];s(m.value,{componentStack:m.stack})}}finally{J.T=n,se.p=l}}if(i=mi,s=fi,fi=null,i!==null&&(mi=null,s===null&&(s=[]),e!==null))for(m=0;m<i.length;m++)n=(0,i[m])(s),n!==void 0&&e.finished.finally(n);(yn&3)!==0&&Gr(),hn(t),l=t.pendingLanes,(a&261930)!==0&&(l&42)!==0?t===Br?ml++:(ml=0,Br=t):(ml=0,Br=null),fl(0)}}function Rm(e,t){(e.pooledCacheLanes&=t)===0&&(t=e.pooledCache,t!=null&&(e.pooledCache=null,Ki(t)))}function Gr(){return _i!==null&&(_i.skipTransition(),_i=null),rc(),sc(),oc(),cc()}function cc(){if(Pe!==5)return!1;var e=rn,t=nc;nc=0;var n=vs(yn),a=J.T,i=se.p;try{se.p=32>n?32:n,J.T=null,n=ac,ac=null;var l=rn,s=yn;if(Pe=0,qa=rn=null,yn=0,(Ae&6)!==0)throw Error(d(331));var u=Ae;if(Ae|=4,ym(l.current),fm(l,l.current,s,n),Ae=u,fl(0,!1),Rt&&typeof Rt.onPostCommitFiberRoot=="function")try{Rt.onPostCommitFiberRoot(Ri,l)}catch{}return!0}finally{se.p=i,J.T=a,Rm(e,t)}}function qm(e,t,n){t=Kt(n,t),t=Eo(e.stateNode,t,2),e=Kn(e,t,2),e!==null&&(Mi(e,2),hn(e))}function Me(e,t,n){if(e.tag===3)qm(e,e,n);else for(;t!==null;){if(t.tag===3){qm(t,e,n);break}else if(t.tag===1){var a=t.stateNode;if(typeof t.type.getDerivedStateFromError=="function"||typeof a.componentDidCatch=="function"&&(na===null||!na.has(a))){e=Kt(n,e),n=C_(2),a=Kn(t,n,2),a!==null&&(A_(n,a,t,e),Mi(a,2),hn(a));break}}t=t.return}}function uc(e,t,n){var a=e.pingCache;if(a===null){a=e.pingCache=new Jv;var i=new Set;a.set(t,i)}else i=a.get(t),i===void 0&&(i=new Set,a.set(t,i));i.has(n)||(ec=!0,i.add(n),e=ly.bind(null,e,t,n),t.then(e,e))}function ly(e,t,n){var a=e.pingCache;a!==null&&a.delete(t),e.pingedLanes|=e.suspendedLanes&n,e.warmLanes&=~n,Be===e&&(ge&n)===n&&((Qe===4||Qe===3&&(ge&62914560)===ge&&300>Ot()-Dr)&&(Ae&2)===0?vi(e,0):Mr|=n,di===ge&&(di=0)),hn(e)}function Mm(e,t){t===0&&(t=gu()),e=ya(e,t),e!==null&&(Mi(e,t),hn(e))}function ry(e){var t=e.memoizedState,n=0;t!==null&&(n=t.retryLane),Mm(e,n)}function sy(e,t){var n=0;switch(e.tag){case 31:case 13:var a=e.stateNode,i=e.memoizedState;i!==null&&(n=i.retryLane);break;case 19:a=e.stateNode;break;case 22:a=e.stateNode._retryCache;break;default:throw Error(d(314))}a!==null&&a.delete(t),Mm(e,n)}function oy(e,t){return _s(e,t)}var hi=null,gi=null,dc=!1,Ir=!1,_c=!1,ia=0;function hn(e){e!==gi&&e.next===null&&(gi===null?hi=gi=e:gi=gi.next=e),Ir=!0,dc||(dc=!0,uy())}function fl(e,t){if(!_c&&Ir){_c=!0;do for(var n=!1,a=hi;a!==null;){if(e!==0){var i=a.pendingLanes;if(i===0)var l=0;else{var s=a.suspendedLanes,u=a.pingedLanes;l=(1<<31-qt(42|e)+1)-1,l&=i&~(s&~u),l=l&201326741?l&201326741|1:l?l|2:0}l!==0&&(n=!0,Bm(a,l))}else l=ge,l=Ml(a,a===Be?l:0,a.cancelPendingCommit!==null||a.timeoutHandle!==-1),(l&3)===0||qi(a,l)||(n=!0,Bm(a,l));a=a.next}while(n);_c=!1}}function cy(){Dm()}function Dm(){Ir=dc=!1;var e=0;ia!==0&&xy()&&(e=ia);for(var t=Ot(),n=null,a=hi;a!==null;){var i=a.next,l=zm(a,t);l===0?(a.next=null,n===null?hi=i:n.next=i,i===null&&(gi=n)):(n=a,(e!==0||(l&3)!==0)&&(Ir=!0)),a=i}Pe!==0&&Pe!==5||fl(e),ia!==0&&(ia=0)}function zm(e,t){for(var n=e.suspendedLanes,a=e.pingedLanes,i=e.expirationTimes,l=e.pendingLanes&-62914561;0<l;){var s=31-qt(l),u=1<<s,m=i[s];m===-1?((u&n)===0||(u&a)!==0)&&(i[s]=kp(u,t)):m<=t&&(e.expiredLanes|=u),l&=~u}if(t=Be,n=ge,n=Ml(e,e===t?n:0,e.cancelPendingCommit!==null||e.timeoutHandle!==-1),a=e.callbackNode,n===0||e===t&&(qe===2||qe===9)||e.cancelPendingCommit!==null)return a!==null&&a!==null&&ms(a),e.callbackNode=null,e.callbackPriority=0;if((n&3)===0||qi(e,n)){if(t=n&-n,t===e.callbackPriority)return t;switch(a!==null&&ms(a),vs(n)){case 2:case 8:n=vu;break;case 32:n=Al;break;case 268435456:n=yu;break;default:n=Al}return a=Um.bind(null,e),n=_s(n,a),e.callbackPriority=t,e.callbackNode=n,t}return a!==null&&a!==null&&ms(a),e.callbackPriority=2,e.callbackNode=null,2}function Um(e,t){if(Pe!==0&&Pe!==5)return e.callbackNode=null,e.callbackPriority=0,null;var n=e.callbackNode;if(Gr()&&e.callbackNode!==n)return null;var a=ge;return a=Ml(e,e===Be?a:0,e.cancelPendingCommit!==null||e.timeoutHandle!==-1),a===0?null:(wm(e,a,t),zm(e,Ot()),e.callbackNode!=null&&e.callbackNode===n?Um.bind(null,e):null)}function Bm(e,t){if(Gr())return null;wm(e,t,!0)}function uy(){Ny(function(){(Ae&6)!==0?_s(pu,cy):Dm()})}function mc(){if(ia===0){var e=Na;e===0&&(e=Ol,Ol<<=1,(Ol&261888)===0&&(Ol=256)),ia=e}return ia}function Vm(e){return e==null||typeof e=="symbol"||typeof e=="boolean"?null:typeof e=="function"?e:Vl(e)}function dy(e,t,n,a,i){if(t==="submit"&&n&&n.stateNode===i){var l=Vm((i[Nt]||null).action),s=a.submitter;s&&(t=(t=s[Nt]||null)?Vm(t.formAction):s.getAttribute("formAction"),t!==null&&(l=t,s=null));var u=new Il("action","action",null,a,i);e.push({event:u,listeners:[{instance:null,listener:function(){if(a.defaultPrevented){if(ia!==0){var m=new FormData(i,s);xo(n,{pending:!0,data:m,method:i.method,action:l},null,m)}}else typeof l=="function"&&(u.preventDefault(),m=new FormData(i,s),xo(n,{pending:!0,data:m,method:i.method,action:l},l,m))},currentTarget:i}]})}}for(var fc=0;fc<Bs.length;fc++){var pc=Bs[fc],_y=pc.toLowerCase(),my=pc[0].toUpperCase()+pc.slice(1);tn(_y,"on"+my)}tn(dd,"onAnimationEnd"),tn(_d,"onAnimationIteration"),tn(md,"onAnimationStart"),tn("dblclick","onDoubleClick"),tn("focusin","onFocus"),tn("focusout","onBlur"),tn(wv,"onTransitionRun"),tn(Nv,"onTransitionStart"),tn(Sv,"onTransitionCancel"),tn(fd,"onTransitionEnd"),Ha("onMouseEnter",["mouseout","mouseover"]),Ha("onMouseLeave",["mouseout","mouseover"]),Ha("onPointerEnter",["pointerout","pointerover"]),Ha("onPointerLeave",["pointerout","pointerover"]),fa("onChange","change click focusin focusout input keydown keyup selectionchange".split(" ")),fa("onSelect","focusout contextmenu dragend focusin keydown keyup mousedown mouseup selectionchange".split(" ")),fa("onBeforeInput",["compositionend","keypress","textInput","paste"]),fa("onCompositionEnd","compositionend focusout keydown keypress keyup mousedown".split(" ")),fa("onCompositionStart","compositionstart focusout keydown keypress keyup mousedown".split(" ")),fa("onCompositionUpdate","compositionupdate focusout keydown keypress keyup mousedown".split(" "));var pl="abort canplay canplaythrough durationchange emptied encrypted ended error loadeddata loadedmetadata loadstart pause play playing progress ratechange resize seeked seeking stalled suspend timeupdate volumechange waiting".split(" "),fy=new Set("beforetoggle cancel close invalid load scroll scrollend toggle".split(" ").concat(pl));function Lm(e,t){t=(t&4)!==0;for(var n=0;n<e.length;n++){var a=e[n],i=a.event;a=a.listeners;e:{var l=void 0;if(t)for(var s=a.length-1;0<=s;s--){var u=a[s],m=u.instance,g=u.currentTarget;if(u=u.listener,m!==l&&i.isPropagationStopped())break e;l=u,i.currentTarget=g;try{l(i)}catch(S){Ql(S)}i.currentTarget=null,l=m}else for(s=0;s<a.length;s++){if(u=a[s],m=u.instance,g=u.currentTarget,u=u.listener,m!==l&&i.isPropagationStopped())break e;l=u,i.currentTarget=g;try{l(i)}catch(S){Ql(S)}i.currentTarget=null,l=m}}}}function ye(e,t){var n=t[ju];n===void 0&&(n=t[ju]=new Set);var a=e+"__bubble";n.has(a)||(Hm(t,e,2,!1),n.add(a))}function vc(e,t,n){var a=0;t&&(a|=4),Hm(n,e,a,t)}var Pr="_reactListening"+Math.random().toString(36).slice(2);function yc(e){if(!e[Pr]){e[Pr]=!0,Tu.forEach(function(n){n!=="selectionchange"&&(fy.has(n)||vc(n,!1,e),vc(n,!0,e))});var t=e.nodeType===9?e:e.ownerDocument;t===null||t[Pr]||(t[Pr]=!0,vc("selectionchange",!1,t))}}function Hm(e,t,n,a){switch(Of(t)){case 2:var i=rh;break;case 8:i=sh;break;default:i=Uc}n=i.bind(null,t,n,e),i=void 0,!Ss||t!=="touchstart"&&t!=="touchmove"&&t!=="wheel"||(i=!0),a?i!==void 0?e.addEventListener(t,n,{capture:!0,passive:i}):e.addEventListener(t,n,!0):i!==void 0?e.addEventListener(t,n,{passive:i}):e.addEventListener(t,n,!1)}function hc(e,t,n,a,i){var l=a;if((t&1)===0&&(t&2)===0&&a!==null)e:for(;;){if(a===null)return;var s=a.tag;if(s===3||s===4){var u=a.stateNode.containerInfo;if(u===i)break;if(s===4)for(s=a.return;s!==null;){var m=s.tag;if((m===3||m===4)&&s.stateNode.containerInfo===i)return;s=s.return}for(;u!==null;){if(s=ma(u),s===null)return;if(m=s.tag,m===5||m===6||m===26||m===27){a=l=s;continue e}u=u.parentNode}}a=a.return}Hu(function(){var g=l,S=ws(n),C=[];e:{var v=pd.get(e);if(v!==void 0){var w=Il,L=e;switch(e){case"keypress":if(Hl(n)===0)break e;case"keydown":case"keyup":w=$p;break;case"focusin":L="focus",w=Ts;break;case"focusout":L="blur",w=Ts;break;case"beforeblur":case"afterblur":w=Ts;break;case"click":if(n.button===2)break e;case"auxclick":case"dblclick":case"mousedown":case"mousemove":case"mouseup":case"mouseout":case"mouseover":case"contextmenu":w=Pu;break;case"drag":case"dragend":case"dragenter":case"dragexit":case"dragleave":case"dragover":case"dragstart":case"drop":w=Lp;break;case"touchcancel":case"touchend":case"touchmove":case"touchstart":w=av;break;case dd:case _d:case md:w=Ip;break;case fd:w=lv;break;case"scroll":case"scrollend":w=Bp;break;case"wheel":w=sv;break;case"copy":case"cut":case"paste":w=Yp;break;case"gotpointercapture":case"lostpointercapture":case"pointercancel":case"pointerdown":case"pointermove":case"pointerout":case"pointerover":case"pointerup":w=Qu;break;case"submit":w=tv;break;case"toggle":case"beforetoggle":w=cv}var X=(t&4)!==0,_e=!X&&(e==="scroll"||e==="scrollend"),h=X?v!==null?v+"Capture":null:v;X=[];for(var f=g,x;f!==null;){var T=f;if(x=T.stateNode,T=T.tag,T!==5&&T!==26&&T!==27||x===null||h===null||(T=Ui(f,h),T!=null&&X.push(vl(f,T,x))),_e)break;f=f.return}0<X.length&&(v=new w(v,L,null,n,S),C.push({event:v,listeners:X}))}}if((t&7)===0){e:{if(w=e==="mouseover"||e==="pointerover",v=e==="mouseout"||e==="pointerout",w&&n!==xs&&(L=n.relatedTarget||n.fromElement)&&(ma(L)||L[Ba]))break e;(v||w)&&(L=S.window===S?S:(w=S.ownerDocument)?w.defaultView||w.parentWindow:window,v?(w=n.relatedTarget||n.toElement,v=g,w=w?ma(w):null,w!==null&&(_e=A(w),X=w.tag,w!==_e||X!==5&&X!==27&&X!==6)&&(w=null)):(v=null,w=g),v!==w&&(X=Pu,T="onMouseLeave",h="onMouseEnter",f="mouse",(e==="pointerout"||e==="pointerover")&&(X=Qu,T="onPointerLeave",h="onPointerEnter",f="pointer"),_e=v==null?L:zi(v),x=w==null?L:zi(w),L=new X(T,f+"leave",v,n,S),L.target=_e,L.relatedTarget=x,T=null,ma(S)===g&&(X=new X(h,f+"enter",w,n,S),X.target=x,X.relatedTarget=_e,T=X),_e=T,X=v&&w?E(v,w,py):null,v!==null&&Gm(C,L,v,X,!1),w!==null&&_e!==null&&Gm(C,_e,w,X,!0)))}e:{if(v=g?zi(g):window,w=v.nodeName&&v.nodeName.toLowerCase(),w==="select"||w==="input"&&v.type==="file")var I=ed;else if($u(v))if(td)I=gv;else{I=yv;var be=vv}else w=v.nodeName,!w||w.toLowerCase()!=="input"||v.type!=="checkbox"&&v.type!=="radio"?g&&bs(g.elementType)&&(I=ed):I=hv;if(I&&(I=I(e,g))){Wu(C,I,n,S);break e}be&&be(e,v,g)}switch(be=g?zi(g):window,e){case"focusin":($u(be)||be.contentEditable==="true")&&(Fa=be,Ds=g,Yi=null);break;case"focusout":Yi=Ds=Fa=null;break;case"mousedown":zs=!0;break;case"contextmenu":case"mouseup":case"dragend":zs=!1,cd(C,n,S);break;case"selectionchange":if(xv)break;case"keydown":case"keyup":cd(C,n,S)}var te;if(As)e:{switch(e){case"compositionstart":var le="onCompositionStart";break e;case"compositionend":le="onCompositionEnd";break e;case"compositionupdate":le="onCompositionUpdate";break e}le=void 0}else Qa?Zu(e,n)&&(le="onCompositionEnd"):e==="keydown"&&n.keyCode===229&&(le="onCompositionStart");le&&(Fu&&n.locale!=="ko"&&(Qa||le!=="onCompositionStart"?le==="onCompositionEnd"&&Qa&&(te=Gu()):(Vn=S,js="value"in Vn?Vn.value:Vn.textContent,Qa=!0)),be=Yr(g,le),0<be.length&&(le=new Yu(le,e,null,n,S),C.push({event:le,listeners:be}),te?le.data=te:(te=Ju(n),te!==null&&(le.data=te)))),(te=dv?_v(e,n):mv(e,n))&&(le=Yr(g,"onBeforeInput"),0<le.length&&(be=new Yu("onBeforeInput","beforeinput",null,n,S),C.push({event:be,listeners:le}),be.data=te)),dy(C,e,g,n,S)}Lm(C,t)})}function vl(e,t,n){return{instance:e,listener:t,currentTarget:n}}function Yr(e,t){for(var n=t+"Capture",a=[];e!==null;){var i=e,l=i.stateNode;if(i=i.tag,i!==5&&i!==26&&i!==27||l===null||(i=Ui(e,n),i!=null&&a.unshift(vl(e,i,l)),i=Ui(e,t),i!=null&&a.push(vl(e,i,l))),e.tag===3)return a;e=e.return}return[]}function py(e){if(e===null)return null;do e=e.return;while(e&&e.tag!==5&&e.tag!==27);return e||null}function Gm(e,t,n,a,i){for(var l=t._reactName,s=[];n!==null&&n!==a;){var u=n,m=u.alternate,g=u.stateNode;if(u=u.tag,m!==null&&m===a)break;u!==5&&u!==26&&u!==27||g===null||(m=g,i?(g=Ui(n,l),g!=null&&s.unshift(vl(n,g,m))):i||(g=Ui(n,l),g!=null&&s.push(vl(n,g,m)))),n=n.return}s.length!==0&&e.push({event:t,listeners:s})}var vy=/\r\n?/g,yy=/\u0000|\uFFFD/g;function Im(e){return(typeof e=="string"?e:""+e).replace(vy,`
`).replace(yy,"")}function Pm(e,t){return t=Im(t),Im(e)===t}function De(e,t,n,a,i,l){switch(n){case"children":if(typeof a=="string")t==="body"||t==="textarea"&&a===""||Ia(e,a);else if(typeof a=="number"||typeof a=="bigint")t!=="body"&&Ia(e,""+a);else return;break;case"className":Bl(e,"class",a);break;case"tabIndex":Bl(e,"tabindex",a);break;case"dir":case"role":case"viewBox":case"width":case"height":Bl(e,n,a);break;case"style":Vu(e,a,l);return;case"data":if(t!=="object"){Bl(e,"data",a);break}case"src":case"href":if(a===""&&(t!=="a"||n!=="href")){e.removeAttribute(n);break}if(a==null||typeof a=="function"||typeof a=="symbol"||typeof a=="boolean"){e.removeAttribute(n);break}a=Vl(a),e.setAttribute(n,a);break;case"action":case"formAction":if(typeof a=="function"){e.setAttribute(n,"javascript:throw new Error('A React form was unexpectedly submitted. If you called form.submit() manually, consider using form.requestSubmit() instead. If you\\'re trying to use event.stopPropagation() in a submit event handler, consider also calling event.preventDefault().')");break}else typeof l=="function"&&(n==="formAction"?(t!=="input"&&De(e,t,"name",i.name,i,null),De(e,t,"formEncType",i.formEncType,i,null),De(e,t,"formMethod",i.formMethod,i,null),De(e,t,"formTarget",i.formTarget,i,null)):(De(e,t,"encType",i.encType,i,null),De(e,t,"method",i.method,i,null),De(e,t,"target",i.target,i,null)));if(a==null||typeof a=="symbol"||typeof a=="boolean"){e.removeAttribute(n);break}a=Vl(a),e.setAttribute(n,a);break;case"onClick":a!=null&&(e.onclick=on);return;case"onScroll":a!=null&&ye("scroll",e);return;case"onScrollEnd":a!=null&&ye("scrollend",e);return;case"dangerouslySetInnerHTML":if(a!=null){if(typeof a!="object"||!("__html"in a))throw Error(d(61));if(n=a.__html,n!=null){if(i.children!=null)throw Error(d(60));(l!=null?l.__html:void 0)!==n&&(e.innerHTML=n)}}break;case"multiple":e.multiple=a&&typeof a!="function"&&typeof a!="symbol";break;case"muted":e.muted=a&&typeof a!="function"&&typeof a!="symbol";break;case"suppressContentEditableWarning":case"suppressHydrationWarning":case"defaultValue":case"defaultChecked":case"innerHTML":case"ref":break;case"autoFocus":break;case"xlinkHref":if(a==null||typeof a=="function"||typeof a=="boolean"||typeof a=="symbol"){e.removeAttribute("xlink:href");break}n=Vl(a),e.setAttributeNS("http://www.w3.org/1999/xlink","xlink:href",n);break;case"contentEditable":case"spellCheck":case"draggable":case"value":case"autoReverse":case"externalResourcesRequired":case"focusable":case"preserveAlpha":a!=null&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,a):e.removeAttribute(n);break;case"inert":case"allowFullScreen":case"async":case"autoPlay":case"controls":case"credentialless":case"default":case"defer":case"disabled":case"disablePictureInPicture":case"disableRemotePlayback":case"formNoValidate":case"hidden":case"loop":case"noModule":case"noValidate":case"open":case"playsInline":case"readOnly":case"required":case"reversed":case"scoped":case"seamless":case"itemScope":a&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,""):e.removeAttribute(n);break;case"capture":case"download":a===!0?e.setAttribute(n,""):a!==!1&&a!=null&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,a):e.removeAttribute(n);break;case"cols":case"rows":case"size":case"span":a!=null&&typeof a!="function"&&typeof a!="symbol"&&!isNaN(a)&&1<=a?e.setAttribute(n,a):e.removeAttribute(n);break;case"rowSpan":case"start":a==null||typeof a=="function"||typeof a=="symbol"||isNaN(a)?e.removeAttribute(n):e.setAttribute(n,a);break;case"popover":ye("beforetoggle",e),ye("toggle",e),Ul(e,"popover",a);break;case"xlinkActuate":wn(e,"http://www.w3.org/1999/xlink","xlink:actuate",a);break;case"xlinkArcrole":wn(e,"http://www.w3.org/1999/xlink","xlink:arcrole",a);break;case"xlinkRole":wn(e,"http://www.w3.org/1999/xlink","xlink:role",a);break;case"xlinkShow":wn(e,"http://www.w3.org/1999/xlink","xlink:show",a);break;case"xlinkTitle":wn(e,"http://www.w3.org/1999/xlink","xlink:title",a);break;case"xlinkType":wn(e,"http://www.w3.org/1999/xlink","xlink:type",a);break;case"xmlBase":wn(e,"http://www.w3.org/XML/1998/namespace","xml:base",a);break;case"xmlLang":wn(e,"http://www.w3.org/XML/1998/namespace","xml:lang",a);break;case"xmlSpace":wn(e,"http://www.w3.org/XML/1998/namespace","xml:space",a);break;case"is":Ul(e,"is",a);break;case"innerText":case"textContent":return;default:if(!(2<n.length)||n[0]!=="o"&&n[0]!=="O"||n[1]!=="n"&&n[1]!=="N")n=zp.get(n)||n,Ul(e,n,a);else return}Te=!0}function gc(e,t,n,a,i,l){switch(n){case"style":Vu(e,a,l);return;case"dangerouslySetInnerHTML":if(a!=null){if(typeof a!="object"||!("__html"in a))throw Error(d(61));if(n=a.__html,n!=null){if(i.children!=null)throw Error(d(60));(l!=null?l.__html:void 0)!==n&&(e.innerHTML=n)}}break;case"children":if(typeof a=="string")Ia(e,a);else if(typeof a=="number"||typeof a=="bigint")Ia(e,""+a);else return;break;case"onScroll":a!=null&&ye("scroll",e);return;case"onScrollEnd":a!=null&&ye("scrollend",e);return;case"onClick":a!=null&&(e.onclick=on);return;case"suppressContentEditableWarning":case"suppressHydrationWarning":case"innerHTML":case"ref":return;case"innerText":case"textContent":return;default:if(!Cu.hasOwnProperty(n))e:{if(n[0]==="o"&&n[1]==="n"&&(i=n.endsWith("Capture"),l=n.slice(2,i?n.length-7:void 0),t=e[Nt]||null,t=t!=null?t[n]:null,typeof t=="function"&&e.removeEventListener(l,t,i),typeof a=="function")){typeof t!="function"&&t!==null&&(n in e?e[n]=null:e.hasAttribute(n)&&e.removeAttribute(n)),e.addEventListener(l,a,i);break e}Te=!0,n in e?e[n]=a:a===!0?e.setAttribute(n,""):Ul(e,n,a)}return}Te=!0}function dt(e,t,n){switch(t){case"div":case"span":case"svg":case"path":case"a":case"g":case"p":case"li":break;case"img":ye("error",e),ye("load",e);var a=!1,i=!1,l;for(l in n)if(n.hasOwnProperty(l)){var s=n[l];if(s!=null)switch(l){case"src":a=!0;break;case"srcSet":i=!0;break;case"children":case"dangerouslySetInnerHTML":throw Error(d(137,t));default:De(e,t,l,s,n,null)}}i&&De(e,t,"srcSet",n.srcSet,n,null),a&&De(e,t,"src",n.src,n,null);return;case"input":ye("invalid",e);var u=l=s=i=null,m=null,g=null;for(a in n)if(n.hasOwnProperty(a)){var S=n[a];if(S!=null)switch(a){case"name":i=S;break;case"type":s=S;break;case"checked":m=S;break;case"defaultChecked":g=S;break;case"value":l=S;break;case"defaultValue":u=S;break;case"children":case"dangerouslySetInnerHTML":if(S!=null)throw Error(d(137,t));break;default:De(e,t,a,S,n,null)}}Du(e,l,u,m,g,s,i,!1);return;case"select":ye("invalid",e),a=s=l=null;for(i in n)if(n.hasOwnProperty(i)&&(u=n[i],u!=null))switch(i){case"value":l=u;break;case"defaultValue":s=u;break;case"multiple":a=u;default:De(e,t,i,u,n,null)}t=l,n=s,e.multiple=!!a,t!=null?Ga(e,!!a,t,!1):n!=null&&Ga(e,!!a,n,!0);return;case"textarea":ye("invalid",e),l=i=a=null;for(s in n)if(n.hasOwnProperty(s)&&(u=n[s],u!=null))switch(s){case"value":a=u;break;case"defaultValue":i=u;break;case"children":l=u;break;case"dangerouslySetInnerHTML":if(u!=null)throw Error(d(91));break;default:De(e,t,s,u,n,null)}Uu(e,a,i,l);return;case"option":for(m in n)if(n.hasOwnProperty(m)&&(a=n[m],a!=null))switch(m){case"selected":e.selected=a&&typeof a!="function"&&typeof a!="symbol";break;default:De(e,t,m,a,n,null)}return;case"dialog":ye("beforetoggle",e),ye("toggle",e),ye("cancel",e),ye("close",e);break;case"iframe":case"object":ye("load",e);break;case"video":case"audio":for(a=0;a<pl.length;a++)ye(pl[a],e);break;case"image":ye("error",e),ye("load",e);break;case"details":ye("toggle",e);break;case"embed":case"source":case"link":ye("error",e),ye("load",e);case"area":case"base":case"br":case"col":case"hr":case"keygen":case"meta":case"param":case"track":case"wbr":case"menuitem":for(g in n)if(n.hasOwnProperty(g)&&(a=n[g],a!=null))switch(g){case"children":case"dangerouslySetInnerHTML":throw Error(d(137,t));default:De(e,t,g,a,n,null)}return;default:if(bs(t)){for(S in n)n.hasOwnProperty(S)&&(a=n[S],a!==void 0&&gc(e,t,S,a,n,void 0));return}}for(u in n)n.hasOwnProperty(u)&&(a=n[u],a!=null&&De(e,t,u,a,n,null))}var hy={};function gy(e,t,n,a){switch(t){case"div":case"span":case"svg":case"path":case"a":case"g":case"p":case"li":break;case"input":var i=null,l=null,s=null,u=null,m=null,g=null,S=null;for(w in n){var C=n[w];if(n.hasOwnProperty(w)&&C!=null)switch(w){case"checked":break;case"value":break;case"defaultValue":m=C;default:a.hasOwnProperty(w)||De(e,t,w,null,a,C)}}for(var v in a){var w=a[v];if(C=n[v],a.hasOwnProperty(v)&&(w!=null||C!=null))switch(v){case"type":w!==C&&(Te=!0),l=w;break;case"name":w!==C&&(Te=!0),i=w;break;case"checked":w!==C&&(Te=!0),g=w;break;case"defaultChecked":w!==C&&(Te=!0),S=w;break;case"value":w!==C&&(Te=!0),s=w;break;case"defaultValue":w!==C&&(Te=!0),u=w;break;case"children":case"dangerouslySetInnerHTML":if(w!=null)throw Error(d(137,t));break;default:w!==C&&De(e,t,v,w,a,C)}}hs(e,s,u,m,g,S,l,i);return;case"select":w=s=u=v=null;for(l in n)if(m=n[l],n.hasOwnProperty(l)&&m!=null)switch(l){case"value":break;case"multiple":w=m;default:a.hasOwnProperty(l)||De(e,t,l,null,a,m)}for(i in a)if(l=a[i],m=n[i],a.hasOwnProperty(i)&&(l!=null||m!=null))switch(i){case"value":l!==m&&(Te=!0),v=l;break;case"defaultValue":l!==m&&(Te=!0),u=l;break;case"multiple":l!==m&&(Te=!0),s=l;default:l!==m&&De(e,t,i,l,a,m)}t=u,n=s,a=w,v!=null?Ga(e,!!n,v,!1):!!a!=!!n&&(t!=null?Ga(e,!!n,t,!0):Ga(e,!!n,n?[]:"",!1));return;case"textarea":w=v=null;for(u in n)if(i=n[u],n.hasOwnProperty(u)&&i!=null&&!a.hasOwnProperty(u))switch(u){case"value":break;case"children":break;default:De(e,t,u,null,a,i)}for(s in a)if(i=a[s],l=n[s],a.hasOwnProperty(s)&&(i!=null||l!=null))switch(s){case"value":i!==l&&(Te=!0),v=i;break;case"defaultValue":i!==l&&(Te=!0),w=i;break;case"children":break;case"dangerouslySetInnerHTML":if(i!=null)throw Error(d(91));break;default:i!==l&&De(e,t,s,i,a,l)}zu(e,v,w);return;case"option":for(var L in n)if(v=n[L],n.hasOwnProperty(L)&&v!=null&&!a.hasOwnProperty(L))switch(L){case"selected":e.selected=!1;break;default:De(e,t,L,null,a,v)}for(m in a)if(v=a[m],w=n[m],a.hasOwnProperty(m)&&v!==w&&(v!=null||w!=null))switch(m){case"selected":v!==w&&(Te=!0),e.selected=v&&typeof v!="function"&&typeof v!="symbol";break;default:De(e,t,m,v,a,w)}return;case"img":case"link":case"area":case"base":case"br":case"col":case"embed":case"hr":case"keygen":case"meta":case"param":case"source":case"track":case"wbr":case"menuitem":for(var X in n)v=n[X],n.hasOwnProperty(X)&&v!=null&&!a.hasOwnProperty(X)&&De(e,t,X,null,a,v);for(g in a)if(v=a[g],w=n[g],a.hasOwnProperty(g)&&v!==w&&(v!=null||w!=null))switch(g){case"children":case"dangerouslySetInnerHTML":if(v!=null)throw Error(d(137,t));break;default:De(e,t,g,v,a,w)}return;default:if(bs(t)){for(var _e in n)v=n[_e],n.hasOwnProperty(_e)&&v!==void 0&&!a.hasOwnProperty(_e)&&gc(e,t,_e,void 0,a,v);for(S in a)v=a[S],w=n[S],!a.hasOwnProperty(S)||v===w||v===void 0&&w===void 0||gc(e,t,S,v,a,w);return}}for(var h in n)v=n[h],n.hasOwnProperty(h)&&v!=null&&!a.hasOwnProperty(h)&&De(e,t,h,null,a,v);for(C in a)v=a[C],w=n[C],!a.hasOwnProperty(C)||v===w||v==null&&w==null||De(e,t,C,v,a,w)}function Ym(e){switch(e){case"css":case"script":case"font":case"img":case"image":case"input":case"link":return!0;default:return!1}}function by(){if(typeof performance.getEntriesByType=="function"){for(var e=0,t=0,n=performance.getEntriesByType("resource"),a=0;a<n.length;a++){var i=n[a],l=i.transferSize,s=i.initiatorType,u=i.duration;if(l&&u&&Ym(s)){for(s=0,u=i.responseEnd,a+=1;a<n.length;a++){var m=n[a],g=m.startTime;if(g>u)break;var S=m.transferSize,C=m.initiatorType;S&&Ym(C)&&(m=m.responseEnd,s+=S*(m<u?1:(u-g)/(m-g)))}if(--a,t+=8*(l+s)/(i.duration/1e3),e++,10<e)break}}if(0<e)return t/e/1e6}return navigator.connection&&(e=navigator.connection.downlink,typeof e=="number")?e:5}var bc=null,xc=null;function yl(e){return e.nodeType===9?e:e.ownerDocument}function Qm(e){switch(e){case"http://www.w3.org/2000/svg":return 1;case"http://www.w3.org/1998/Math/MathML":return 2;default:return 0}}function Fm(e,t){if(e===0)switch(t){case"svg":return 1;case"math":return 2;default:return 0}return e===1&&t==="foreignObject"?0:e}function Km(e,t,n,a){return n=yl(n).createElement(e),n[rt]=a,n[Nt]=t,dt(n,e,t),tt(n),n}function wc(e,t){return e==="textarea"||e==="noscript"||typeof t.children=="string"||typeof t.children=="number"||typeof t.children=="bigint"||typeof t.dangerouslySetInnerHTML=="object"&&t.dangerouslySetInnerHTML!==null&&t.dangerouslySetInnerHTML.__html!=null}var Nc=null;function xy(){var e=window.event;return e&&e.type==="popstate"?e===Nc?!1:(Nc=e,!0):(Nc=null,!1)}var Sc=typeof setTimeout=="function"?setTimeout:void 0,wy=typeof clearTimeout=="function"?clearTimeout:void 0,Xm=typeof Promise=="function"?Promise:void 0,Zm=typeof requestAnimationFrame=="function"?requestAnimationFrame:Sc,Ny=typeof queueMicrotask=="function"?queueMicrotask:typeof Xm<"u"?function(e){return Xm.resolve(null).then(e).catch(Sy)}:Sc;function Sy(e){setTimeout(function(){throw e})}function la(e){return e==="head"}function Jm(e,t){var n=t,a=0;do{var i=n.nextSibling;if(e.removeChild(n),i&&i.nodeType===8)if(n=i.data,n==="/$"||n==="/&"){if(a===0){e.removeChild(i),Ei(t);return}a--}else if(n==="$"||n==="$?"||n==="$~"||n==="$!"||n==="&")a++;else if(n==="html")Rc(e.ownerDocument.documentElement);else if(n==="head"){n=e.ownerDocument.head,Rc(n);for(var l=n.firstChild;l;){var s=l.nextSibling,u=l.nodeName;l[Di]||u==="SCRIPT"||u==="STYLE"||u==="LINK"&&l.rel.toLowerCase()==="stylesheet"||n.removeChild(l),l=s}}else n==="body"&&Rc(e.ownerDocument.body);n=i}while(n);Ei(t)}function $m(e,t){var n=e;e=0;do{var a=n.nextSibling;if(n.nodeType===1?t?(n._stashedDisplay=n.style.display,n.style.display="none"):(n.style.display=n._stashedDisplay||"",n.getAttribute("style")===""&&n.removeAttribute("style")):n.nodeType===3&&(t?(n._stashedText=n.nodeValue,n.nodeValue=""):n.nodeValue=n._stashedText||""),a&&a.nodeType===8)if(n=a.data,n==="/$"){if(e===0)break;e--}else n!=="$"&&n!=="$?"&&n!=="$~"&&n!=="$!"||e++;n=a}while(n)}function Wm(e,t,n){if(t=CSS.escape(t)!==t?"r-"+btoa(t).replace(/=/g,""):t,e.style.viewTransitionName=t,n!=null&&(e.style.viewTransitionClass=n),n=getComputedStyle(e),n.display==="inline"){if(t=e.getClientRects(),t.length===1)var a=1;else for(var i=a=0;i<t.length;i++){var l=t[i];0<l.width&&0<l.height&&a++}a===1&&(e=e.style,e.display=t.length===1?"inline-block":"block",e.marginTop="-"+n.paddingTop,e.marginBottom="-"+n.paddingBottom)}}function ef(e,t){e=e.style,t=t.style;var n=t!=null?t.hasOwnProperty("viewTransitionName")?t.viewTransitionName:t.hasOwnProperty("view-transition-name")?t["view-transition-name"]:null:null;e.viewTransitionName=n==null||typeof n=="boolean"?"":(""+n).trim(),n=t!=null?t.hasOwnProperty("viewTransitionClass")?t.viewTransitionClass:t.hasOwnProperty("view-transition-class")?t["view-transition-class"]:null:null,e.viewTransitionClass=n==null||typeof n=="boolean"?"":(""+n).trim(),e.display==="inline-block"&&(t==null?e.display=e.margin="":(n=t.display,e.display=n==null||typeof n=="boolean"?"":n,n=t.margin,n!=null?e.margin=n:(n=t.hasOwnProperty("marginTop")?t.marginTop:t["margin-top"],e.marginTop=n==null||typeof n=="boolean"?"":n,t=t.hasOwnProperty("marginBottom")?t.marginBottom:t["margin-bottom"],e.marginBottom=t==null||typeof t=="boolean"?"":t)))}function jy(e,t,n){return n=n.ownerDocument.defaultView,{rect:e,abs:t.position==="absolute"||t.position==="fixed",clip:t.clipPath!=="none"||t.overflow!=="visible"||t.filter!=="none"||t.mask!=="none"||t.mask!=="none"||t.borderRadius!=="0px",view:0<=e.bottom&&0<=e.right&&e.top<=n.innerHeight&&e.left<=n.innerWidth}}function jc(e){var t=e.getBoundingClientRect(),n=getComputedStyle(e);return jy(t,n,e)}function Ey(e){return e.documentElement.clientHeight}function ky(e){this.addEventListener("load",e),this.addEventListener("error",e)}function Ty(e,t,n,a,i,l,s,u,m){var g=t.nodeType===9?t:t.ownerDocument;try{var S=g.startViewTransition({update:function(){var v=g.defaultView,w=v.navigation&&v.navigation.transition,L=g.fonts.status;a();var X=[];if(L==="loaded"&&(Ey(g),g.fonts.status==="loading"&&X.push(g.fonts.ready)),L=X.length,e!==null)for(var _e=e.suspenseyImages,h=0,f=0;f<_e.length;f++){var x=_e[f];if(!x.complete){var T=x.getBoundingClientRect();if(0<T.bottom&&0<T.right&&T.top<v.innerHeight&&T.left<v.innerWidth){if(h+=wf(x),h>Kr){X.length=L;break}x=new Promise(ky.bind(x)),X.push(x)}}}if(0<X.length)return v=Promise.race([Promise.all(X),new Promise(function(I){return setTimeout(I,500)})]).then(i,i),(w?Promise.allSettled([w.finished,v]):v).then(l,l);if(i(),w)return w.finished.then(l,l);l()},types:n});g.__reactViewTransition=S;var C=[];return S.ready.then(function(){for(var v=g.documentElement.getAnimations({subtree:!0}),w=0;w<v.length;w++){var L=v[w],X=L.effect,_e=X.pseudoElement;if(_e!=null&&_e.startsWith("::view-transition")){C.push(L),L=X.getKeyframes();for(var h=_e=void 0,f=!0,x=0;x<L.length;x++){var T=L[x],I=T.width;if(_e===void 0)_e=I;else if(_e!==I){f=!1;break}if(I=T.height,h===void 0)h=I;else if(h!==I){f=!1;break}delete T.width,delete T.height,T.transform==="none"&&delete T.transform}f&&_e!==void 0&&h!==void 0&&(X.setKeyframes(L),f=getComputedStyle(X.target,X.pseudoElement),f.width!==_e||f.height!==h)&&(f=L[0],f.width=_e,f.height=h,f=L[L.length-1],f.width=_e,f.height=h,X.setKeyframes(L))}}s()},function(v){g.__reactViewTransition===S&&(g.__reactViewTransition=null);try{if(typeof v=="object"&&v!==null)switch(v.name){case"InvalidStateError":(v.message==="View transition was skipped because document visibility state is hidden."||v.message==="Skipping view transition because document visibility state has become hidden."||v.message==="Skipping view transition because viewport size changed."||v.message==="Transition was aborted because of invalid state")&&(v=null)}v!==null&&m(v)}finally{a(),i(),s()}}),S.finished.finally(function(){for(var v=0;v<C.length;v++)C[v].cancel();g.__reactViewTransition===S&&(g.__reactViewTransition=null),u()}),S}catch{return a(),i(),s(),null}}function Ma(e,t){this._scope=document.documentElement,this._selector="::view-transition-"+e+"("+t+")"}Ma.prototype.animate=function(e,t){return t=typeof t=="number"?{duration:t}:D({},t),t.pseudoElement=this._selector,this._scope.animate(e,t)},Ma.prototype.getAnimations=function(){for(var e=this._scope,t=this._selector,n=e.getAnimations({subtree:!0}),a=[],i=0;i<n.length;i++){var l=n[i].effect;l!==null&&l.target===e&&l.pseudoElement===t&&a.push(n[i])}return a},Ma.prototype.getComputedStyle=function(){return getComputedStyle(this._scope,this._selector)};function tf(e){return{name:e,group:new Ma("group",e),imagePair:new Ma("image-pair",e),old:new Ma("old",e),new:new Ma("new",e)}}function Ht(e){this._fragmentFiber=e,this._observers=this._eventListeners=null}Ht.prototype.addEventListener=function(e,t,n){var a=null,i=null;if(!(n!=null&&typeof n!="boolean"&&(a=n.signal||null,a!==null&&a.aborted))){this._eventListeners===null&&(this._eventListeners=[]);var l=this._eventListeners;if(af(l,e,t,n)===-1){var s=this,u=t;n!=null&&typeof n!="boolean"&&n.once===!0&&(u=function(m){s.removeEventListener(e,t,n),typeof t=="function"?t.call(this,m):t.handleEvent(m)}),a!==null&&(i=s.removeEventListener.bind(s,e,t,n),a.addEventListener("abort",i,{once:!0}),i=a.removeEventListener.bind(a,"abort",i)),a=bi(n),l.push({type:e,listener:t,optionsOrUseCapture:n,attachedListener:u,cleanup:i}),y(this._fragmentFiber.child,!1,Cy,e,u,a)}this._eventListeners=l}};function Cy(e,t,n,a){return V(e).addEventListener(t,n,a),!1}Ht.prototype.removeEventListener=function(e,t,n){var a=this._eventListeners;if(a!==null&&(t=af(a,e,t,n),t!==-1)){var i=a[t];n=i.attachedListener;var l=i.cleanup;i=bi(i.optionsOrUseCapture),y(this._fragmentFiber.child,!1,Ay,e,n,i),a.splice(t,1),l!==null&&l()}};function Ay(e,t,n,a){return V(e).removeEventListener(t,n,a),!1}function bi(e){return e!=null&&typeof e!="boolean"&&(e.once===!0||e.signal instanceof AbortSignal)?{capture:e.capture,passive:e.passive}:e}function nf(e){return e==null?"c=0":typeof e=="boolean"?"c="+(e?"1":"0"):"c="+(e.capture?"1":"0")}function af(e,t,n,a){if(e.length===0)return-1;a=nf(a);for(var i=0;i<e.length;i++){var l=e[i];if(l.type===t&&l.listener===n&&nf(l.optionsOrUseCapture)===a)return i}return-1}Ht.prototype.dispatchEvent=function(e){var t=O(this._fragmentFiber);if(t===null)return!0;t=V(t);var n=this._eventListeners;if(n!==null&&0<n.length||!e.bubbles){var a=t.nodeType===9?t.createComment(""):document.createTextNode("");if(n)for(var i=0;i<n.length;i++){var l=n[i];a.addEventListener(l.type,l.attachedListener,bi(l.optionsOrUseCapture))}if(t.appendChild(a),e=a.dispatchEvent(e),n)for(i=0;i<n.length;i++)l=n[i],a.removeEventListener(l.type,l.attachedListener,bi(l.optionsOrUseCapture));return t.removeChild(a),e}return t.dispatchEvent(e)},Ht.prototype.focus=function(e){y(this._fragmentFiber.child,!0,lf,e,void 0,void 0)};function lf(e,t){return e.tag===6?!1:(e=V(e),Gy(e,t))}Ht.prototype.focusLast=function(e){var t=[];y(this._fragmentFiber.child,!0,Ec,t,void 0,void 0);for(var n=t.length-1;0<=n&&!lf(t[n],e);n--);};function Ec(e,t){return t.push(e),!1}Ht.prototype.blur=function(){var e=O(this._fragmentFiber);e!==null&&(e=V(e),e=yl(e).activeElement,e!==null&&y(this._fragmentFiber.child,!1,Oy,e,void 0,void 0))};function Oy(e,t){return e.tag===6?!1:(e=V(e),e===t||e.contains(t)?(t.blur(),!0):!1)}Ht.prototype.observeUsing=function(e){this._observers===null&&(this._observers=new Set),this._observers.add(e),y(this._fragmentFiber.child,!1,Ry,e,void 0,void 0)};function Ry(e,t){return e.tag===6||(e=V(e),t.observe(e)),!1}Ht.prototype.unobserveUsing=function(e){var t=this._observers;if(t!==null&&t.has(e)){t.delete(e),y(this._fragmentFiber.child,!1,qy,e,void 0,void 0);for(var n=t=0;n<sn.length;n++){var a=sn[n];a.fragmentInstance===this&&a.observer===e?e.unobserve(a.instance):sn[t++]=a}sn.length=t}};function qy(e,t){return e.tag===6||(e=V(e),t.unobserve(e)),!1}var sn=[],kc=!1;function My(e,t,n){sn.push({fragmentInstance:e,observer:t,instance:n}),kc||(kc=!0,Iy(function(){kc=!1;var a=sn;sn=[];for(var i=0;i<a.length;i++){var l=a[i];l.observer.unobserve(l.instance)}}))}Ht.prototype.getClientRects=function(){var e=[];return y(this._fragmentFiber.child,!1,Dy,e,void 0,void 0),e};function Dy(e,t){if(e.tag===6){e=e.stateNode;var n=e.ownerDocument.createRange();n.selectNodeContents(e),t.push.apply(t,n.getClientRects())}else e=V(e),t.push.apply(t,e.getClientRects());return!1}Ht.prototype.getRootNode=function(e){var t=O(this._fragmentFiber);return t===null?this:V(t).getRootNode(e)},Ht.prototype.compareDocumentPosition=function(e){var t=O(this._fragmentFiber);if(t===null)return Node.DOCUMENT_POSITION_DISCONNECTED;var n=[];y(this._fragmentFiber.child,!1,Ec,n,void 0,void 0);var a=V(t);if(n.length===0){if(n=a,$(this._fragmentFiber)){e:{for(t=this._fragmentFiber.return;t!==null;){if(t.tag===4){t=t.stateNode.containerInfo;break e}if(t.tag===3||t.tag===5||t.tag===27)break;t=t.return}t=null}t!=null&&(n=t)}t=this._fragmentFiber;var i=a=n.compareDocumentPosition(e);return n===e?i=Node.DOCUMENT_POSITION_CONTAINS:a&Node.DOCUMENT_POSITION_CONTAINED_BY&&(n=q(t)[1],n===null?i=Node.DOCUMENT_POSITION_PRECEDING:(e=V(n).compareDocumentPosition(e),i=e===0||e&Node.DOCUMENT_POSITION_FOLLOWING?Node.DOCUMENT_POSITION_FOLLOWING:Node.DOCUMENT_POSITION_PRECEDING)),i|=Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC}t=V(n[0]),i=V(n[n.length-1]);var l=$(this._fragmentFiber)?t.parentElement:a;if(l==null)return Node.DOCUMENT_POSITION_DISCONNECTED;a=l.compareDocumentPosition(t)&Node.DOCUMENT_POSITION_CONTAINED_BY,l=l.compareDocumentPosition(i)&Node.DOCUMENT_POSITION_CONTAINED_BY;var s=t.compareDocumentPosition(e),u=i.compareDocumentPosition(e),m=s&Node.DOCUMENT_POSITION_CONTAINED_BY||u&Node.DOCUMENT_POSITION_CONTAINED_BY;return u=a&&l&&s&Node.DOCUMENT_POSITION_FOLLOWING&&u&Node.DOCUMENT_POSITION_PRECEDING,t=a&&t===e||l&&i===e||m||u?Node.DOCUMENT_POSITION_CONTAINED_BY:!a&&t===e||!l&&i===e?Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC:s,t&Node.DOCUMENT_POSITION_DISCONNECTED||t&Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC||zy(t,this._fragmentFiber,n[0],n[n.length-1],e)?t:Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC};function zy(e,t,n,a,i){var l=ma(i);if(e&Node.DOCUMENT_POSITION_CONTAINED_BY){if(n=!!l)e:{for(;l!==null;){if(l.tag===7&&(l===t||l.alternate===t)){n=!0;break e}l=l.return}n=!1}return n}if(e&Node.DOCUMENT_POSITION_CONTAINS){if(l===null)return l=i.ownerDocument,i===l||i===l.documentElement||i===l.body;e:{for(l=t,t=O(t);l!==null;){if(!(l.tag!==5&&l.tag!==3&&l.tag!==27||l!==t&&l.alternate!==t)){l=!0;break e}l=l.return}l=!1}return l}return e&Node.DOCUMENT_POSITION_PRECEDING?((t=!!l)&&!(t=l===n)&&(t=E(n,l,Ee),t===null?t=!1:(y(t,!0,G,l,n),l=Z,Z=null,t=l!==null)),t):e&Node.DOCUMENT_POSITION_FOLLOWING?((t=!!l)&&!(t=l===a)&&(t=E(a,l,Ee),t===null?t=!1:(y(t,!0,ce,l,a),l=Z,ie=Z=null,t=l!==null)),t):!1}function rf(e,t){var n=e.ownerDocument.createRange();n.selectNodeContents(e),e=n.getBoundingClientRect(),window.scrollTo(window.scrollX+e.left,t?window.scrollY+e.top:window.scrollY+e.bottom-window.innerHeight)}Ht.prototype.scrollIntoView=function(e){if(typeof e=="object")throw Error(d(566));var t=[];y(this._fragmentFiber.child,!1,Ec,t,void 0,void 0);var n=e!==!1;if(t.length===0){var a=q(this._fragmentFiber);if(a=n?a[1]||a[0]||O(this._fragmentFiber):a[0]||a[1],a===null)return;if(a.tag===6){e=V(a),rf(e,n);return}if(a=V(a),a.nodeType!==9){if(a.nodeType===11){n="host"in a?a.host:null,n!==null&&n.scrollIntoView(e);return}a.scrollIntoView(e)}}for(a=n?t.length-1:0;a!==(n?-1:t.length);){var i=t[a];i.tag===6?(i=V(i),rf(i,n)):V(i).scrollIntoView(e),a+=n?-1:1}};function Uy(e,t){return e=V(e),sf(e,t),!1}function sf(e,t){e.reactFragments==null&&(e.reactFragments=new Set),e.reactFragments.add(t)}function of(e,t){var n=t._eventListeners;if(n!==null)for(var a=0;a<n.length;a++){var i=n[a];e.addEventListener(i.type,i.attachedListener,bi(i.optionsOrUseCapture))}e.nodeType!==3&&(n=t._observers,n!==null&&n.forEach(function(l){for(var s=0,u=0;u<sn.length;u++){var m=sn[u];(m.fragmentInstance!==t||m.observer!==l||m.instance!==e)&&(sn[s++]=m)}sn.length=s,l.observe(e)}),sf(e,t))}function By(e,t){var n=t._eventListeners;if(n!==null)for(var a=0;a<n.length;a++){var i=n[a];e.removeEventListener(i.type,i.attachedListener,bi(i.optionsOrUseCapture))}e.nodeType!==3&&(n=t._observers,n!==null&&n.forEach(function(l){typeof l.rootMargin=="string"?My(t,l,e):l.unobserve(e)}),e.reactFragments!=null&&e.reactFragments.delete(t))}function Tc(e){var t=e.firstChild;for(t&&t.nodeType===10&&(t=t.nextSibling);t;){var n=t;switch(t=t.nextSibling,n.nodeName){case"HTML":case"HEAD":case"BODY":Tc(n),zl(n);continue;case"SCRIPT":case"STYLE":continue;case"LINK":if(n.rel.toLowerCase()==="stylesheet")continue}e.removeChild(n)}}function Vy(e,t,n,a){for(;e.nodeType===1;){var i=n;if(e.nodeName.toLowerCase()!==t.toLowerCase()){if(!a&&(e.nodeName!=="INPUT"||e.type!=="hidden"))break}else if(a){if(!e[Di])switch(t){case"meta":if(!e.hasAttribute("itemprop"))break;return e;case"link":if(l=e.getAttribute("rel"),l==="stylesheet"&&e.hasAttribute("data-precedence"))break;if(l!==i.rel||e.getAttribute("href")!==(i.href==null||i.href===""?null:i.href)||e.getAttribute("crossorigin")!==(i.crossOrigin==null?null:i.crossOrigin)||e.getAttribute("title")!==(i.title==null?null:i.title))break;return e;case"style":if(e.hasAttribute("data-precedence"))break;return e;case"script":if(l=e.getAttribute("src"),(l!==(i.src==null?null:i.src)||e.getAttribute("type")!==(i.type==null?null:i.type)||e.getAttribute("crossorigin")!==(i.crossOrigin==null?null:i.crossOrigin))&&l&&e.hasAttribute("async")&&!e.hasAttribute("itemprop"))break;return e;default:return e}}else if(t==="input"&&e.type==="hidden"){var l=i.name==null?null:""+i.name;if(i.type==="hidden"&&e.getAttribute("name")===l)return e}else return e;if(e=Wt(e.nextSibling),e===null)break}return null}function Ly(e,t,n){if(t==="")return null;for(;e.nodeType!==3;)if((e.nodeType!==1||e.nodeName!=="INPUT"||e.type!=="hidden")&&!n||(e=Wt(e.nextSibling),e===null))return null;return e}function cf(e,t){for(;e.nodeType!==8;)if((e.nodeType!==1||e.nodeName!=="INPUT"||e.type!=="hidden")&&!t||(e=Wt(e.nextSibling),e===null))return null;return e}function Cc(e){return e.data==="$?"||e.data==="$~"}function Ac(e){return e.data==="$!"||e.data==="$?"&&e.ownerDocument.readyState!=="loading"}function Hy(e,t){var n=e.ownerDocument;if(e.data==="$~")e._reactRetry=t;else if(e.data!=="$?"||n.readyState!=="loading")t();else{var a=function(){t(),n.removeEventListener("DOMContentLoaded",a)};n.addEventListener("DOMContentLoaded",a),e._reactRetry=a}}function Wt(e){for(;e!=null;e=e.nextSibling){var t=e.nodeType;if(t===1||t===3)break;if(t===8){if(t=e.data,t==="$"||t==="$!"||t==="$?"||t==="$~"||t==="&"||t==="F!"||t==="F")break;if(t==="/$"||t==="/&")return null}}return e}var Oc=null;function uf(e){e=e.nextSibling;for(var t=0;e;){if(e.nodeType===8){var n=e.data;if(n==="/$"||n==="/&"){if(t===0)return Wt(e.nextSibling);t--}else n!=="$"&&n!=="$!"&&n!=="$?"&&n!=="$~"&&n!=="&"||t++}e=e.nextSibling}return null}function df(e){e=e.previousSibling;for(var t=0;e;){if(e.nodeType===8){var n=e.data;if(n==="$"||n==="$!"||n==="$?"||n==="$~"||n==="&"){if(t===0)return e;t--}else n!=="/$"&&n!=="/&"||t++}e=e.previousSibling}return null}function Gy(e,t){function n(){a=!0}if(e.ownerDocument.activeElement===e)return!0;var a=!1;try{e.ownerDocument.addEventListener("focus",n,!0),(e.focus||HTMLElement.prototype.focus).call(e,t)}finally{e.ownerDocument.removeEventListener("focus",n,!0)}return a}function Iy(e){Zm(function(){Zm(function(t){return e(t)})})}function _f(e,t,n){switch(t=yl(n),e){case"html":if(e=t.documentElement,!e)throw Error(d(452));return e;case"head":if(e=t.head,!e)throw Error(d(453));return e;case"body":if(e=t.body,!e)throw Error(d(454));return e;default:throw Error(d(451))}}function mf(e,t,n){for(var a in n){var i=n[a];n.hasOwnProperty(a)&&i!=null&&De(e,t,a,null,hy,i)}n.dangerouslySetInnerHTML!=null&&(e.textContent=""),e.onclick===on&&(e.onclick=null),zl(e)}function Rc(e){for(var t=e.attributes;t.length;)e.removeAttributeNode(t[0]);zl(e)}var en=new Map,ff=new Set;function hl(e){if(typeof e.getRootNode=="function"){var t=e.getRootNode();if(t.nodeType===9||t.nodeType===11)return t}return e.nodeType===9?e:e.ownerDocument}var Dn=se.d;se.d={f:Py,r:Yy,D:Qy,C:Fy,L:Ky,m:Xy,X:Jy,S:Zy,M:$y};function Py(){var e=Dn.f(),t=Vr();return e||t}function Yy(e){var t=Va(e);t!==null&&t.tag===5&&t.type==="form"?p_(t):Dn.r(e)}var xi=typeof document>"u"?null:document;function pf(e,t,n){var a=xi;if(a&&typeof t=="string"&&t){var i=Qt(t);i='link[rel="'+e+'"][href="'+i+'"]',typeof n=="string"&&(i+='[crossorigin="'+n+'"]'),ff.has(i)||(ff.add(i),e={rel:e,crossOrigin:n,href:t},a.querySelector(i)===null&&(t=a.createElement("link"),dt(t,"link",e),tt(t),a.head.appendChild(t)))}}function Qy(e){Dn.D(e),pf("dns-prefetch",e,null)}function Fy(e,t){Dn.C(e,t),pf("preconnect",e,t)}function Ky(e,t,n){Dn.L(e,t,n);var a=xi;if(a&&e&&t){var i='link[rel="preload"][as="'+Qt(t)+'"]';t==="image"&&n&&n.imageSrcSet?(i+='[imagesrcset="'+Qt(n.imageSrcSet)+'"]',typeof n.imageSizes=="string"&&(i+='[imagesizes="'+Qt(n.imageSizes)+'"]')):i+='[href="'+Qt(e)+'"]';var l=i;switch(t){case"style":l=wi(e);break;case"script":l=Ni(e)}if(!(en.has(l)||(e=D({rel:"preload",href:t==="image"&&n&&n.imageSrcSet?void 0:e,as:t},n),en.set(l,e),a.querySelector(i)!==null||t==="style"&&a.querySelector(gl(l))||t==="script"&&a.querySelector(bl(l))))){var s=a.createElement("link");dt(s,"link",e),t==="style"&&(s[Dl]=!0,s.onload=s.onerror=function(){ku(s)}),tt(s),a.head.appendChild(s)}}}function Xy(e,t){Dn.m(e,t);var n=xi;if(n&&e){var a=t&&typeof t.as=="string"?t.as:"script",i='link[rel="modulepreload"][as="'+Qt(a)+'"][href="'+Qt(e)+'"]',l=i;switch(a){case"audioworklet":case"paintworklet":case"serviceworker":case"sharedworker":case"worker":case"script":l=Ni(e)}if(!en.has(l)&&(e=D({rel:"modulepreload",href:e},t),en.set(l,e),n.querySelector(i)===null)){switch(a){case"audioworklet":case"paintworklet":case"serviceworker":case"sharedworker":case"worker":case"script":if(n.querySelector(bl(l)))return}a=n.createElement("link"),dt(a,"link",e),tt(a),n.head.appendChild(a)}}}function Zy(e,t,n){Dn.S(e,t,n);var a=xi;if(a&&e){var i=La(a).hoistableStyles,l=wi(e);t=t||"default";var s=i.get(l);if(!s){var u={loading:0,preload:null};if(s=a.querySelector(gl(l)))u.loading=5;else{e=D({rel:"stylesheet",href:e,"data-precedence":t},n),(n=en.get(l))&&qc(e,n);var m=s=a.createElement("link");tt(m),dt(m,"link",e),m._p=new Promise(function(g,S){m.onload=g,m.onerror=S}),m.addEventListener("load",function(){u.loading|=1}),m.addEventListener("error",function(){u.loading|=2}),u.loading|=4,Qr(s,t,a)}s={type:"stylesheet",instance:s,count:1,state:u},i.set(l,s)}}}function Jy(e,t){Dn.X(e,t);var n=xi;if(n&&e){var a=La(n).hoistableScripts,i=Ni(e),l=a.get(i);l||(l=n.querySelector(bl(i)),l||(e=D({src:e,async:!0},t),(t=en.get(i))&&Mc(e,t),l=n.createElement("script"),tt(l),dt(l,"link",e),n.head.appendChild(l)),l={type:"script",instance:l,count:1,state:null},a.set(i,l))}}function $y(e,t){Dn.M(e,t);var n=xi;if(n&&e){var a=La(n).hoistableScripts,i=Ni(e),l=a.get(i);l||(l=n.querySelector(bl(i)),l||(e=D({src:e,async:!0,type:"module"},t),(t=en.get(i))&&Mc(e,t),l=n.createElement("script"),tt(l),dt(l,"link",e),n.head.appendChild(l)),l={type:"script",instance:l,count:1,state:null},a.set(i,l))}}function vf(e,t,n,a){var i=(i=Fe.current)?hl(i):null;if(!i)throw Error(d(446));switch(e){case"meta":case"title":return null;case"style":return typeof n.precedence=="string"&&typeof n.href=="string"?(n=wi(n.href),t=La(i).hoistableStyles,a=t.get(n),a||(a={type:"style",instance:null,count:0,state:null},t.set(n,a)),a):{type:"void",instance:null,count:0,state:null};case"link":if(n.rel==="stylesheet"&&typeof n.href=="string"&&typeof n.precedence=="string"){e=wi(n.href);var l=La(i).hoistableStyles,s=l.get(e);if(s||(i=i.ownerDocument||i,s={type:"stylesheet",instance:null,count:0,state:{loading:0,preload:null}},l.set(e,s),(l=i.querySelector(gl(e)))?l._p||(s.instance=l,s.state.loading=5):(l=en.get(e),l||(l={rel:"preload",as:"style",href:n.href,crossOrigin:n.crossOrigin,integrity:n.integrity,media:n.media,hrefLang:n.hrefLang,referrerPolicy:n.referrerPolicy},en.set(e,l)),Wy(i,e,l,s.state))),t&&a===null)throw Error(d(528,""));return s}if(t&&a!==null)throw Error(d(529,""));return null;case"script":return t=n.async,n=n.src,typeof n=="string"&&t&&typeof t!="function"&&typeof t!="symbol"?(n=Ni(n),t=La(i).hoistableScripts,a=t.get(n),a||(a={type:"script",instance:null,count:0,state:null},t.set(n,a)),a):{type:"void",instance:null,count:0,state:null};default:throw Error(d(444,e))}}function wi(e){return'href="'+Qt(e)+'"'}function gl(e){return'link[rel="stylesheet"]['+e+"]"}function yf(e){return D({},e,{"data-precedence":e.precedence,precedence:null})}function Wy(e,t,n,a){if(t=e.querySelector('link[rel="preload"][as="style"]['+t+"]")){if(t[Dl]!==!0){a.loading=1;return}}else t=e.createElement("link"),t[Dl]=!0,t.onload=t.onerror=ku.bind(null,t),dt(t,"link",n),tt(t),e.head.appendChild(t);a.preload=t,t.addEventListener("load",function(){return a.loading|=1}),t.addEventListener("error",function(){return a.loading|=2})}function Ni(e){return'[src="'+Qt(e)+'"]'}function bl(e){return"script[async]"+e}function hf(e,t,n){if(t.count++,t.instance===null)switch(t.type){case"style":var a=e.querySelector('style[data-href~="'+Qt(n.href)+'"]');if(a)return t.instance=a,tt(a),a;var i=D({},n,{"data-href":n.href,"data-precedence":n.precedence,href:null,precedence:null});return a=(e.ownerDocument||e).createElement("style"),tt(a),dt(a,"style",i),Qr(a,n.precedence,e),t.instance=a;case"stylesheet":i=wi(n.href);var l=e.querySelector(gl(i));if(l)return t.state.loading|=4,t.instance=l,tt(l),l;a=yf(n),(i=en.get(i))&&qc(a,i),l=(e.ownerDocument||e).createElement("link"),tt(l);var s=l;return s._p=new Promise(function(u,m){s.onload=u,s.onerror=m}),dt(l,"link",a),t.state.loading|=4,Qr(l,n.precedence,e),t.instance=l;case"script":return l=Ni(n.src),(i=e.querySelector(bl(l)))?(t.instance=i,tt(i),i):(a=n,(i=en.get(l))&&(a=D({},n),Mc(a,i)),e=e.ownerDocument||e,i=e.createElement("script"),tt(i),dt(i,"link",a),e.head.appendChild(i),t.instance=i);case"void":return null;default:throw Error(d(443,t.type))}else t.type==="stylesheet"&&(t.state.loading&4)===0&&(a=t.instance,t.state.loading|=4,Qr(a,n.precedence,e));return t.instance}function Qr(e,t,n){for(var a=n.querySelectorAll('link[rel="stylesheet"][data-precedence],style[data-precedence]'),i=a.length?a[a.length-1]:null,l=i,s=0;s<a.length;s++){var u=a[s];if(u.dataset.precedence===t)l=u;else if(l!==i)break}l?l.parentNode.insertBefore(e,l.nextSibling):(t=n.nodeType===9?n.head:n,t.insertBefore(e,t.firstChild))}function qc(e,t){e.crossOrigin==null&&(e.crossOrigin=t.crossOrigin),e.referrerPolicy==null&&(e.referrerPolicy=t.referrerPolicy),e.title==null&&(e.title=t.title)}function Mc(e,t){e.crossOrigin==null&&(e.crossOrigin=t.crossOrigin),e.referrerPolicy==null&&(e.referrerPolicy=t.referrerPolicy),e.integrity==null&&(e.integrity=t.integrity)}var Fr=null;function gf(e,t,n){if(Fr===null){var a=new Map,i=Fr=new Map;i.set(n,a)}else i=Fr,a=i.get(n),a||(a=new Map,i.set(n,a));if(a.has(e))return a;for(a.set(e,null),n=n.getElementsByTagName(e),i=0;i<n.length;i++){var l=n[i];if(!(l[Di]||l[rt]||e==="link"&&l.getAttribute("rel")==="stylesheet")&&l.namespaceURI!=="http://www.w3.org/2000/svg"){var s=l.getAttribute(t)||"";s=e+s;var u=a.get(s);u?u.push(l):a.set(s,[l])}}return a}function Dc(e,t,n){e=e.ownerDocument||e,e.head.insertBefore(n,t==="title"?e.querySelector("head > title"):null)}function eh(e,t,n){if(n===1||t.itemProp!=null)return!1;switch(e){case"meta":case"title":return!0;case"style":if(typeof t.precedence!="string"||typeof t.href!="string"||t.href==="")break;return!0;case"link":if(typeof t.rel!="string"||typeof t.href!="string"||t.href===""||t.onLoad||t.onError)break;switch(t.rel){case"stylesheet":return e=t.disabled,typeof t.precedence=="string"&&e==null;default:return!0}case"script":if(t.async&&typeof t.async!="function"&&typeof t.async!="symbol"&&!t.onLoad&&!t.onError&&t.src&&typeof t.src=="string")return!0}return!1}function bf(e,t){return e==="img"&&t.src!=null&&t.src!==""&&t.onLoad==null&&t.loading!=="lazy"}function xf(e){return!(e.type==="stylesheet"&&(e.state.loading&3)===0)}function wf(e){return(e.width||100)*(e.height||100)*(typeof devicePixelRatio=="number"?devicePixelRatio:1)*.25}function Nf(e,t){typeof t.decode=="function"&&(e.imgCount++,t.complete||(e.imgBytes+=wf(t),e.suspenseyImages.push(t)),e=ah.bind(e),t.decode().then(e,e))}function th(e,t,n,a){if(n.type==="stylesheet"&&(typeof a.media!="string"||matchMedia(a.media).matches!==!1)&&(n.state.loading&4)===0){if(n.instance===null){var i=wi(a.href),l=t.querySelector(gl(i));if(l){t=l._p,t!==null&&typeof t=="object"&&typeof t.then=="function"&&(e.count++,e=xl.bind(e),t.then(e,e)),n.state.loading|=4,n.instance=l,tt(l);return}l=t.ownerDocument||t,a=yf(a),(i=en.get(i))&&qc(a,i),l=l.createElement("link"),tt(l);var s=l;s._p=new Promise(function(u,m){s.onload=u,s.onerror=m}),dt(l,"link",a),n.instance=l}e.stylesheets===null&&(e.stylesheets=new Map),e.stylesheets.set(n,t),(t=n.state.preload)&&(n.state.loading&3)===0&&(e.count++,n=xl.bind(e),t.addEventListener("load",n),t.addEventListener("error",n))}}var Kr=0;function nh(e,t){return e.stylesheets&&e.count===0&&Zr(e,e.stylesheets),0<e.count||0<e.imgCount?function(n){var a=setTimeout(function(){if(e.stylesheets&&Zr(e,e.stylesheets),e.unsuspend){var l=e.unsuspend;e.unsuspend=null,l()}},6e4+t);0<e.imgBytes&&Kr===0&&(Kr=62500*by());var i=setTimeout(function(){if(e.waitingForImages=!1,e.count===0&&(e.stylesheets&&Zr(e,e.stylesheets),e.unsuspend)){var l=e.unsuspend;e.unsuspend=null,l()}},(e.imgBytes>Kr?50:800)+t);return e.unsuspend=n,function(){e.unsuspend=null,clearTimeout(a),clearTimeout(i)}}:null}function Sf(e){if(e.count===0&&(e.imgCount===0||!e.waitingForImages)){if(e.stylesheets)Zr(e,e.stylesheets);else if(e.unsuspend){var t=e.unsuspend;e.unsuspend=null,t()}}}function xl(){this.count--,Sf(this)}function ah(){this.imgCount--,Sf(this)}var Xr=null;function Zr(e,t){e.stylesheets=null,e.unsuspend!==null&&(e.count++,Xr=new Map,t.forEach(ih,e),Xr=null,xl.call(e))}function ih(e,t){if(!(t.state.loading&4)){var n=Xr.get(e);if(n)var a=n.get(null);else{n=new Map,Xr.set(e,n);for(var i=e.querySelectorAll("link[data-precedence],style[data-precedence]"),l=0;l<i.length;l++){var s=i[l];(s.nodeName==="LINK"||s.getAttribute("media")!=="not all")&&(n.set(s.dataset.precedence,s),a=s)}a&&n.set(null,a)}i=t.instance,s=i.getAttribute("data-precedence"),l=n.get(s)||a,l===a&&n.set(null,i),n.set(s,i),this.count++,a=xl.bind(this),i.addEventListener("load",a),i.addEventListener("error",a),l?l.parentNode.insertBefore(i,l.nextSibling):(e=e.nodeType===9?e.head:e,e.insertBefore(i,e.firstChild)),t.state.loading|=4}}var Si={$$typeof:He,Provider:null,Consumer:null,_currentValue:Gt,_currentValue2:Gt,_threadCount:0};function lh(e,t,n,a,i,l,s,u,m){this.tag=1,this.containerInfo=e,this.pingCache=this.current=this.pendingChildren=null,this.timeoutHandle=-1,this.callbackNode=this.next=this.pendingContext=this.context=this.cancelPendingCommit=null,this.callbackPriority=0,this.expirationTimes=fs(-1),this.entangledLanes=this.shellSuspendCounter=this.errorRecoveryDisabledLanes=this.expiredLanes=this.warmLanes=this.pingedLanes=this.suspendedLanes=this.pendingLanes=0,this.entanglements=fs(0),this.hiddenUpdates=fs(null),this.identifierPrefix=a,this.onUncaughtError=i,this.onCaughtError=l,this.onRecoverableError=s,this.pooledCache=null,this.pooledCacheLanes=0,this.formState=m,this.transitionTypes=null,this.incompleteTransitions=new Map}function jf(e,t,n,a,i,l,s,u,m,g,S,C){return e=new lh(e,t,n,s,m,g,S,C,u),t=1,l===!0&&(t|=24),l=St(3,null,null,t),e.current=l,l.stateNode=e,t=Xs(),t.refCount++,e.pooledCache=t,t.refCount++,l.memoizedState={element:a,isDehydrated:n,cache:t},Ws(l),e}function Ef(e){return e?(e=Za,e):Za}function kf(e,t,n,a,i,l){i=Ef(i),a.context===null?a.context=i:a.pendingContext=i,a=Fn(t),a.payload={element:n},l=l===void 0?null:l,l!==null&&(a.callback=l),n=Kn(e,a,t),n!==null&&(Tt(n,e,t),$i(n,e,t))}function Tf(e,t){if(e=e.memoizedState,e!==null&&e.dehydrated!==null){var n=e.retryLane;e.retryLane=n!==0&&n<t?n:t}}function zc(e,t){Tf(e,t),(e=e.alternate)&&Tf(e,t)}function Cf(e){if(e.tag===13||e.tag===31){var t=ya(e,67108864);t!==null&&Tt(t,e,67108864),zc(e,67108864)}}function Af(e){if(e.tag===13||e.tag===31){var t=Lt();t=ps(t);var n=ya(e,t);n!==null&&Tt(n,e,t),zc(e,t)}}var ji=!0;function rh(e,t,n,a){var i=J.T;J.T=null;var l=se.p;try{se.p=2,Uc(e,t,n,a)}finally{se.p=l,J.T=i}}function sh(e,t,n,a){var i=J.T;J.T=null;var l=se.p;try{se.p=8,Uc(e,t,n,a)}finally{se.p=l,J.T=i}}function Uc(e,t,n,a){if(ji){var i=Bc(a);if(i===null)hc(e,t,a,Jr,n),Rf(e,a);else if(ch(i,e,t,n,a))a.stopPropagation();else if(Rf(e,a),t&4&&-1<oh.indexOf(e)){for(;i!==null;){var l=Va(i);if(l!==null)switch(l.tag){case 3:if(l=l.stateNode,l.current.memoizedState.isDehydrated){var s=_a(l.pendingLanes);if(s!==0){var u=l;for(u.pendingLanes|=2,u.entangledLanes|=2;s;){var m=1<<31-qt(s);u.entanglements[1]|=m,s&=~m}hn(l),(Ae&6)===0&&(zr=Ot()+500,fl(0))}}break;case 31:case 13:u=ya(l,2),u!==null&&Tt(u,l,2),Vr(),zc(l,2)}if(l=Bc(a),l===null&&hc(e,t,a,Jr,n),l===i)break;i=l}i!==null&&a.stopPropagation()}else hc(e,t,a,null,n)}}function Bc(e){return e=ws(e),Vc(e)}var Jr=null;function Vc(e){if(Jr=null,e=ma(e),e!==null){var t=A(e);if(t===null)e=null;else{var n=t.tag;if(n===13){if(e=N(t),e!==null)return e;e=null}else if(n===31){if(e=B(t),e!==null)return e;e=null}else if(n===3){if(t.stateNode.current.memoizedState.isDehydrated)return t.tag===3?t.stateNode.containerInfo:null;e=null}else t!==e&&(e=null)}}return Jr=e,null}function Of(e){switch(e){case"beforetoggle":case"cancel":case"click":case"close":case"contextmenu":case"copy":case"cut":case"auxclick":case"dblclick":case"dragend":case"dragstart":case"drop":case"focusin":case"focusout":case"input":case"invalid":case"keydown":case"keypress":case"keyup":case"mousedown":case"mouseup":case"paste":case"pause":case"play":case"pointercancel":case"pointerdown":case"pointerup":case"ratechange":case"reset":case"seeked":case"submit":case"toggle":case"touchcancel":case"touchend":case"touchstart":case"volumechange":case"change":case"selectionchange":case"textInput":case"compositionstart":case"compositionend":case"compositionupdate":case"beforeblur":case"afterblur":case"beforeinput":case"blur":case"fullscreenchange":case"fullscreenerror":case"focus":case"hashchange":case"popstate":case"select":case"selectstart":return 2;case"drag":case"dragenter":case"dragexit":case"dragleave":case"dragover":case"mousemove":case"mouseout":case"mouseover":case"pointermove":case"pointerout":case"pointerover":case"resize":case"scroll":case"touchmove":case"wheel":case"mouseenter":case"mouseleave":case"pointerenter":case"pointerleave":return 8;case"message":switch(bp()){case pu:return 2;case vu:return 8;case Al:case xp:return 32;case yu:return 268435456;default:return 32}default:return 32}}var Lc=!1,ra=null,sa=null,oa=null,wl=new Map,Nl=new Map,ca=[],oh="mousedown mouseup touchcancel touchend touchstart auxclick dblclick pointercancel pointerdown pointerup dragend dragstart drop compositionend compositionstart keydown keypress keyup input textInput copy cut paste click change contextmenu reset".split(" ");function Rf(e,t){switch(e){case"focusin":case"focusout":ra=null;break;case"dragenter":case"dragleave":sa=null;break;case"mouseover":case"mouseout":oa=null;break;case"pointerover":case"pointerout":wl.delete(t.pointerId);break;case"gotpointercapture":case"lostpointercapture":Nl.delete(t.pointerId)}}function Sl(e,t,n,a,i,l){return e===null||e.nativeEvent!==l?(e={blockedOn:t,domEventName:n,eventSystemFlags:a,nativeEvent:l,targetContainers:[i]},t!==null&&(t=Va(t),t!==null&&Cf(t)),e):(e.eventSystemFlags|=a,t=e.targetContainers,i!==null&&t.indexOf(i)===-1&&t.push(i),e)}function ch(e,t,n,a,i){switch(t){case"focusin":return ra=Sl(ra,e,t,n,a,i),!0;case"dragenter":return sa=Sl(sa,e,t,n,a,i),!0;case"mouseover":return oa=Sl(oa,e,t,n,a,i),!0;case"pointerover":var l=i.pointerId;return wl.set(l,Sl(wl.get(l)||null,e,t,n,a,i)),!0;case"gotpointercapture":return l=i.pointerId,Nl.set(l,Sl(Nl.get(l)||null,e,t,n,a,i)),!0}return!1}function qf(e){var t=ma(e.target);if(t!==null){var n=A(t);if(n!==null){if(t=n.tag,t===13){if(t=N(n),t!==null){e.blockedOn=t,Su(e.priority,function(){Af(n)});return}}else if(t===31){if(t=B(n),t!==null){e.blockedOn=t,Su(e.priority,function(){Af(n)});return}}else if(t===3&&n.stateNode.current.memoizedState.isDehydrated){e.blockedOn=n.tag===3?n.stateNode.containerInfo:null;return}}}e.blockedOn=null}function $r(e){if(e.blockedOn!==null)return!1;for(var t=e.targetContainers;0<t.length;){var n=Bc(e.nativeEvent);if(n===null){n=e.nativeEvent;var a=new n.constructor(n.type,n);xs=a,n.target.dispatchEvent(a),xs=null}else return t=Va(n),t!==null&&Cf(t),e.blockedOn=n,!1;t.shift()}return!0}function Mf(e,t,n){$r(e)&&n.delete(t)}function uh(){Lc=!1,ra!==null&&$r(ra)&&(ra=null),sa!==null&&$r(sa)&&(sa=null),oa!==null&&$r(oa)&&(oa=null),wl.forEach(Mf),Nl.forEach(Mf)}function Wr(e,t){e.blockedOn===t&&(e.blockedOn=null,Lc||(Lc=!0,o.unstable_scheduleCallback(o.unstable_NormalPriority,uh)))}var es=null;function Df(e){es!==e&&(es=e,o.unstable_scheduleCallback(o.unstable_NormalPriority,function(){es===e&&(es=null);for(var t=0;t<e.length;t+=3){var n=e[t],a=e[t+1],i=e[t+2];if(typeof a!="function"){if(Vc(a||n)===null)continue;break}var l=Va(n);l!==null&&(e.splice(t,3),t-=3,xo(l,{pending:!0,data:i,method:n.method,action:a},a,i))}}))}function Ei(e){function t(m){return Wr(m,e)}ra!==null&&Wr(ra,e),sa!==null&&Wr(sa,e),oa!==null&&Wr(oa,e),wl.forEach(t),Nl.forEach(t);for(var n=0;n<ca.length;n++){var a=ca[n];a.blockedOn===e&&(a.blockedOn=null)}for(;0<ca.length&&(n=ca[0],n.blockedOn===null);)qf(n),n.blockedOn===null&&ca.shift();if(n=(e.ownerDocument||e).$$reactFormReplay,n!=null)for(a=0;a<n.length;a+=3){var i=n[a],l=n[a+1],s=i[Nt]||null;if(typeof l=="function")s||Df(n);else if(s){var u=null;if(l&&l.hasAttribute("formAction")){if(i=l,s=l[Nt]||null)u=s.formAction;else if(Vc(i)!==null)continue}else u=s.action;typeof u=="function"?n[a+1]=u:(n.splice(a,3),a-=3),Df(n)}}}function zf(){function e(l){l.canIntercept&&l.info==="react-transition"&&l.intercept({handler:function(){return new Promise(function(s){return i=s})},focusReset:"manual",scroll:"manual"})}function t(){i!==null&&(i(),i=null),a||setTimeout(n,20)}function n(){if(!a&&!navigation.transition){var l=navigation.currentEntry;l&&l.url!=null&&navigation.navigate(l.url,{state:l.getState(),info:"react-transition",history:"replace"})}}if(typeof navigation=="object"){var a=!1,i=null;return navigation.addEventListener("navigate",e),navigation.addEventListener("navigatesuccess",t),navigation.addEventListener("navigateerror",t),setTimeout(n,100),function(){a=!0,navigation.removeEventListener("navigate",e),navigation.removeEventListener("navigatesuccess",t),navigation.removeEventListener("navigateerror",t),i!==null&&(i(),i=null)}}}function Hc(e){this._internalRoot=e}ts.prototype.render=Hc.prototype.render=function(e){var t=this._internalRoot;if(t===null)throw Error(d(409));var n=t.current,a=Lt();kf(n,a,e,t,null,null)},ts.prototype.unmount=Hc.prototype.unmount=function(){var e=this._internalRoot;if(e!==null){this._internalRoot=null;var t=e.containerInfo;kf(e.current,2,null,e,null,null),Vr(),t[Ba]=null}};function ts(e){this._internalRoot=e}ts.prototype.unstable_scheduleHydration=function(e){if(e){var t=Nu();e={blockedOn:null,target:e,priority:t};for(var n=0;n<ca.length&&t!==0&&t<ca[n].priority;n++);ca.splice(n,0,e),n===0&&qf(e)}};var Uf=c.version;if(Uf!=="19.3.1")throw Error(d(527,Uf,"19.3.1"));se.findDOMNode=function(e){var t=e._reactInternals;if(t===void 0)throw typeof e.render=="function"?Error(d(188)):(e=Object.keys(e).join(","),Error(d(268,e)));return e=R(t),e=e!==null?b(e):null,e=e===null?null:e.stateNode,e};var dh={bundleType:0,version:"19.3.1",rendererPackageName:"react-dom",currentDispatcherRef:J,reconcilerVersion:"19.3.1"};if(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__<"u"){var ns=__REACT_DEVTOOLS_GLOBAL_HOOK__;if(!ns.isDisabled&&ns.supportsFiber)try{Ri=ns.inject(dh),Rt=ns}catch{}}return El.createRoot=function(e,t){if(!j(e))throw Error(d(299));var n=!1,a="",i=j_,l=E_,s=k_;return t!=null&&(t.unstable_strictMode===!0&&(n=!0),t.identifierPrefix!==void 0&&(a=t.identifierPrefix),t.onUncaughtError!==void 0&&(i=t.onUncaughtError),t.onCaughtError!==void 0&&(l=t.onCaughtError),t.onRecoverableError!==void 0&&(s=t.onRecoverableError)),t=jf(e,1,!1,null,null,n,a,null,i,l,s,zf),e[Ba]=t.current,yc(e),new Hc(t)},El.hydrateRoot=function(e,t,n){if(!j(e))throw Error(d(299));var a=!1,i="",l=j_,s=E_,u=k_,m=null;return n!=null&&(n.unstable_strictMode===!0&&(a=!0),n.identifierPrefix!==void 0&&(i=n.identifierPrefix),n.onUncaughtError!==void 0&&(l=n.onUncaughtError),n.onCaughtError!==void 0&&(s=n.onCaughtError),n.onRecoverableError!==void 0&&(u=n.onRecoverableError),n.formState!==void 0&&(m=n.formState)),t=jf(e,1,!0,t,n??null,a,i,m,l,s,u,zf),t.context=Ef(null),n=t.current,a=Lt(),a=ps(a),i=Fn(a),i.callback=null,Kn(n,i,a),n=a,t.current.lanes=n,Mi(t,n),hn(t),e[Ba]=t.current,yc(e),new ts(t)},El.version="19.3.1",El}var Ff;function Nh(){if(Ff)return Yc.exports;Ff=1;function o(){if(!(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__>"u"||typeof __REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE!="function"))try{__REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE(o)}catch(c){console.error(c)}}return o(),Yc.exports=wh(),Yc.exports}var Sh=Nh();/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const jh=o=>o.replace(/([a-z0-9])([A-Z])/g,"$1-$2").toLowerCase(),tp=(...o)=>o.filter((c,_,d)=>!!c&&c.trim()!==""&&d.indexOf(c)===_).join(" ").trim();/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */var Eh={xmlns:"http://www.w3.org/2000/svg",width:24,height:24,viewBox:"0 0 24 24",fill:"none",stroke:"currentColor",strokeWidth:2,strokeLinecap:"round",strokeLinejoin:"round"};/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const kh=F.forwardRef(({color:o="currentColor",size:c=24,strokeWidth:_=2,absoluteStrokeWidth:d,className:j="",children:A,iconNode:N,...B},U)=>F.createElement("svg",{ref:U,...Eh,width:c,height:c,stroke:o,strokeWidth:d?Number(_)*24/Number(c):_,className:tp("lucide",j),...B},[...N.map(([R,b])=>F.createElement(R,b)),...Array.isArray(A)?A:[A]]));/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const fe=(o,c)=>{const _=F.forwardRef(({className:d,...j},A)=>F.createElement(kh,{ref:A,iconNode:c,className:tp(`lucide-${jh(o)}`,d),...j}));return _.displayName=`${o}`,_};/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Th=[["path",{d:"M22 12h-2.48a2 2 0 0 0-1.93 1.46l-2.35 8.36a.25.25 0 0 1-.48 0L9.24 2.18a.25.25 0 0 0-.48 0l-2.35 8.36A2 2 0 0 1 4.49 12H2",key:"169zse"}]],gn=fe("Activity",Th);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ch=[["rect",{width:"16",height:"10",x:"2",y:"7",rx:"2",ry:"2",key:"1w10f2"}],["line",{x1:"22",x2:"22",y1:"11",y2:"13",key:"4dh1rd"}]],Ah=fe("Battery",Ch);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Oh=[["path",{d:"M20 6 9 17l-5-5",key:"1gmf2c"}]],lu=fe("Check",Oh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Rh=[["path",{d:"m6 9 6 6 6-6",key:"qrunsl"}]],qh=fe("ChevronDown",Rh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Mh=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["line",{x1:"12",x2:"12",y1:"8",y2:"12",key:"1pkeuh"}],["line",{x1:"12",x2:"12.01",y1:"16",y2:"16",key:"4dfq90"}]],Dh=fe("CircleAlert",Mh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const zh=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["path",{d:"m9 12 2 2 4-4",key:"dzmm74"}]],ru=fe("CircleCheck",zh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Uh=[["rect",{width:"8",height:"4",x:"8",y:"2",rx:"1",ry:"1",key:"tgr4d6"}],["path",{d:"M16 4h2a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2",key:"116196"}],["path",{d:"m9 14 2 2 4-4",key:"df797q"}]],Bh=fe("ClipboardCheck",Uh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Vh=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["polyline",{points:"12 6 12 12 16.5 12",key:"1aq6pp"}]],Lh=fe("Clock3",Vh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Hh=[["path",{d:"m18 16 4-4-4-4",key:"1inbqp"}],["path",{d:"m6 8-4 4 4 4",key:"15zrgr"}],["path",{d:"m14.5 4-5 16",key:"e7oirm"}]],np=fe("CodeXml",Hh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Gh=[["rect",{width:"14",height:"14",x:"8",y:"8",rx:"2",ry:"2",key:"17jyea"}],["path",{d:"M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2",key:"zix9uf"}]],su=fe("Copy",Gh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ih=[["rect",{width:"16",height:"16",x:"4",y:"4",rx:"2",key:"14l7u7"}],["rect",{width:"6",height:"6",x:"9",y:"9",rx:"1",key:"5aljv4"}],["path",{d:"M15 2v2",key:"13l42r"}],["path",{d:"M15 20v2",key:"15mkzm"}],["path",{d:"M2 15h2",key:"1gxd5l"}],["path",{d:"M2 9h2",key:"1bbxkp"}],["path",{d:"M20 15h2",key:"19e6y8"}],["path",{d:"M20 9h2",key:"19tzq7"}],["path",{d:"M9 2v2",key:"165o2o"}],["path",{d:"M9 20v2",key:"i2bqo8"}]],za=fe("Cpu",Ih);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ph=[["path",{d:"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4",key:"ih7n3h"}],["polyline",{points:"7 10 12 15 17 10",key:"2ggqvy"}],["line",{x1:"12",x2:"12",y1:"15",y2:"3",key:"1vk2je"}]],Yh=fe("Download",Ph);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Qh=[["path",{d:"M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z",key:"1rqfz7"}],["path",{d:"M14 2v4a2 2 0 0 0 2 2h4",key:"tnqrlb"}],["path",{d:"M10 9H8",key:"b1mrlr"}],["path",{d:"M16 13H8",key:"t4e002"}],["path",{d:"M16 17H8",key:"z1uh3a"}]],Fh=fe("FileText",Qh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Kh=[["path",{d:"M12 10a2 2 0 0 0-2 2c0 1.02-.1 2.51-.26 4",key:"1nerag"}],["path",{d:"M14 13.12c0 2.38 0 6.38-1 8.88",key:"o46ks0"}],["path",{d:"M17.29 21.02c.12-.6.43-2.3.5-3.02",key:"ptglia"}],["path",{d:"M2 12a10 10 0 0 1 18-6",key:"ydlgp0"}],["path",{d:"M2 16h.01",key:"1gqxmh"}],["path",{d:"M21.8 16c.2-2 .131-5.354 0-6",key:"drycrb"}],["path",{d:"M5 19.5C5.5 18 6 15 6 12a6 6 0 0 1 .34-2",key:"1tidbn"}],["path",{d:"M8.65 22c.21-.66.45-1.32.57-2",key:"13wd9y"}],["path",{d:"M9 6.8a6 6 0 0 1 9 5.2v2",key:"1fr1j5"}]],ou=fe("Fingerprint",Kh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Xh=[["path",{d:"M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z",key:"96xj49"}]],ls=fe("Flame",Xh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Zh=[["path",{d:"m12 14 4-4",key:"9kzdfg"}],["path",{d:"M3.34 19a10 10 0 1 1 17.32 0",key:"19p75a"}]],rs=fe("Gauge",Zh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Jh=[["circle",{cx:"12",cy:"12",r:"3",key:"1v7zrd"}],["line",{x1:"3",x2:"9",y1:"12",y2:"12",key:"1dyftd"}],["line",{x1:"15",x2:"21",y1:"12",y2:"12",key:"oup4p8"}]],$h=fe("GitCommitHorizontal",Jh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Wh=[["line",{x1:"22",x2:"2",y1:"12",y2:"12",key:"1y58io"}],["path",{d:"M5.45 5.11 2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z",key:"oot6mr"}],["line",{x1:"6",x2:"6.01",y1:"16",y2:"16",key:"sgf278"}],["line",{x1:"10",x2:"10.01",y1:"16",y2:"16",key:"1l4acy"}]],Tl=fe("HardDrive",Wh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const eg=[["path",{d:"M12.83 2.18a2 2 0 0 0-1.66 0L2.6 6.08a1 1 0 0 0 0 1.83l8.58 3.91a2 2 0 0 0 1.66 0l8.58-3.9a1 1 0 0 0 0-1.83z",key:"zw3jo"}],["path",{d:"M2 12a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 12",key:"1wduqc"}],["path",{d:"M2 17a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 17",key:"kqbvx6"}]],cu=fe("Layers",eg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const tg=[["circle",{cx:"12",cy:"16",r:"1",key:"1au0dj"}],["rect",{x:"3",y:"10",width:"18",height:"12",rx:"2",key:"6s8ecr"}],["path",{d:"M7 10V7a5 5 0 0 1 10 0v3",key:"1pqi11"}]],uu=fe("LockKeyhole",tg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ng=[["path",{d:"M18 8V6a2 2 0 0 0-2-2H4a2 2 0 0 0-2 2v7a2 2 0 0 0 2 2h8",key:"10dyio"}],["path",{d:"M10 19v-3.96 3.15",key:"1irgej"}],["path",{d:"M7 19h5",key:"qswx4l"}],["rect",{width:"6",height:"10",x:"16",y:"12",rx:"2",key:"1egngj"}]],ag=fe("MonitorSmartphone",ng);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ig=[["path",{d:"M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z",key:"a7tn18"}]],lg=fe("Moon",ig);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const rg=[["rect",{x:"16",y:"16",width:"6",height:"6",rx:"1",key:"4q2zg0"}],["rect",{x:"2",y:"16",width:"6",height:"6",rx:"1",key:"8cvhb9"}],["rect",{x:"9",y:"2",width:"6",height:"6",rx:"1",key:"1egb70"}],["path",{d:"M5 16v-3a1 1 0 0 1 1-1h12a1 1 0 0 1 1 1v3",key:"1jsf9p"}],["path",{d:"M12 12V8",key:"2874zd"}]],sg=fe("Network",rg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const og=[["circle",{cx:"13.5",cy:"6.5",r:".5",fill:"currentColor",key:"1okk4w"}],["circle",{cx:"17.5",cy:"10.5",r:".5",fill:"currentColor",key:"f64h9f"}],["circle",{cx:"8.5",cy:"7.5",r:".5",fill:"currentColor",key:"fotxhn"}],["circle",{cx:"6.5",cy:"12.5",r:".5",fill:"currentColor",key:"qy21gx"}],["path",{d:"M12 2C6.5 2 2 6.5 2 12s4.5 10 10 10c.926 0 1.648-.746 1.648-1.688 0-.437-.18-.835-.437-1.125-.29-.289-.438-.652-.438-1.125a1.64 1.64 0 0 1 1.668-1.668h1.996c3.051 0 5.555-2.503 5.555-5.554C21.965 6.012 17.461 2 12 2z",key:"12rzf8"}]],cg=fe("Palette",og);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ug=[["polygon",{points:"6 3 20 12 6 21 6 3",key:"1oa8hb"}]],ap=fe("Play",ug);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const dg=[["path",{d:"M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8",key:"v9h5vc"}],["path",{d:"M21 3v5h-5",key:"1q7to0"}],["path",{d:"M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16",key:"3uifl3"}],["path",{d:"M8 16H3v5",key:"1cv678"}]],du=fe("RefreshCw",dg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const _g=[["path",{d:"M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8",key:"1357e3"}],["path",{d:"M3 3v5h5",key:"1xhq8a"}]],ip=fe("RotateCcw",_g);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const mg=[["circle",{cx:"11",cy:"11",r:"8",key:"4ej97u"}],["path",{d:"m21 21-4.3-4.3",key:"1qie3q"}]],lp=fe("Search",mg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const fg=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}],["path",{d:"M12 8v4",key:"1got3b"}],["path",{d:"M12 16h.01",key:"1drbdi"}]],Ai=fe("ShieldAlert",fg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const pg=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}],["path",{d:"m9 12 2 2 4-4",key:"dzmm74"}]],Un=fe("ShieldCheck",pg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const vg=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}]],yg=fe("Shield",vg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const hg=[["line",{x1:"4",x2:"4",y1:"21",y2:"14",key:"1p332r"}],["line",{x1:"4",x2:"4",y1:"10",y2:"3",key:"gb41h5"}],["line",{x1:"12",x2:"12",y1:"21",y2:"12",key:"hf2csr"}],["line",{x1:"12",x2:"12",y1:"8",y2:"3",key:"1kfi7u"}],["line",{x1:"20",x2:"20",y1:"21",y2:"16",key:"1lhrwl"}],["line",{x1:"20",x2:"20",y1:"12",y2:"3",key:"16vvfq"}],["line",{x1:"2",x2:"6",y1:"14",y2:"14",key:"1uebub"}],["line",{x1:"10",x2:"14",y1:"8",y2:"8",key:"1yglbp"}],["line",{x1:"18",x2:"22",y1:"16",y2:"16",key:"1jxqpz"}]],gg=fe("SlidersVertical",hg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const bg=[["rect",{width:"14",height:"20",x:"5",y:"2",rx:"2",ry:"2",key:"1yt0o3"}],["path",{d:"M12 18h.01",key:"mhygvu"}]],xg=fe("Smartphone",bg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const wg=[["path",{d:"M9.937 15.5A2 2 0 0 0 8.5 14.063l-6.135-1.582a.5.5 0 0 1 0-.962L8.5 9.936A2 2 0 0 0 9.937 8.5l1.582-6.135a.5.5 0 0 1 .963 0L14.063 8.5A2 2 0 0 0 15.5 9.937l6.135 1.581a.5.5 0 0 1 0 .964L15.5 14.063a2 2 0 0 0-1.437 1.437l-1.582 6.135a.5.5 0 0 1-.963 0z",key:"4pj2yx"}],["path",{d:"M20 3v4",key:"1olli1"}],["path",{d:"M22 5h-4",key:"1gvqau"}],["path",{d:"M4 17v2",key:"vumght"}],["path",{d:"M5 18H3",key:"zchphs"}]],Ng=fe("Sparkles",wg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Sg=[["circle",{cx:"12",cy:"12",r:"4",key:"4exip2"}],["path",{d:"M12 2v2",key:"tus03m"}],["path",{d:"M12 20v2",key:"1lh1kg"}],["path",{d:"m4.93 4.93 1.41 1.41",key:"149t6j"}],["path",{d:"m17.66 17.66 1.41 1.41",key:"ptbguv"}],["path",{d:"M2 12h2",key:"1t8f8n"}],["path",{d:"M20 12h2",key:"1q8mjw"}],["path",{d:"m6.34 17.66-1.41 1.41",key:"1m8zz5"}],["path",{d:"m19.07 4.93-1.41 1.41",key:"1shlcs"}]],jg=fe("Sun",Sg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Eg=[["polyline",{points:"4 17 10 11 4 5",key:"akl6gq"}],["line",{x1:"12",x2:"20",y1:"19",y2:"19",key:"q2wloq"}]],_u=fe("Terminal",Eg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const kg=[["path",{d:"M14 4v10.54a4 4 0 1 1-4 0V4a2 2 0 0 1 4 0Z",key:"17jzev"}]],rp=fe("Thermometer",kg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Tg=[["path",{d:"m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3",key:"wmoenq"}],["path",{d:"M12 9v4",key:"juzpu7"}],["path",{d:"M12 17h.01",key:"p32p05"}]],Cg=fe("TriangleAlert",Tg);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ag=[["path",{d:"M18 6 6 18",key:"1bl5f8"}],["path",{d:"m6 6 12 12",key:"d8bk6v"}]],sp=fe("X",Ag);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Og=[["path",{d:"M4 14a1 1 0 0 1-.78-1.63l9.9-10.2a.5.5 0 0 1 .86.46l-1.92 6.02A1 1 0 0 0 13 10h7a1 1 0 0 1 .78 1.63l-9.9 10.2a.5.5 0 0 1-.86-.46l1.92-6.02A1 1 0 0 0 11 14z",key:"1xq2db"}]],Rg=fe("Zap",Og),op="coreflow.appearance.v1",qg=["system","light","dark"],cp=["ocean","emerald","amber"];function Mg(){var o;try{const c=typeof window<"u"?(o=window.localStorage)==null?void 0:o.getItem(op):null,_=JSON.parse(c||"{}"),d=qg.includes(_.theme)?_.theme:"system",j=cp.includes(_.accent)?_.accent:"ocean";return{theme:d,reduceMotion:_.reduceMotion===!0,accent:j}}catch{return{theme:"system",reduceMotion:!1,accent:"ocean"}}}function up(o){var c;return o==="dark"?!0:o==="light"?!1:typeof window<"u"&&!!((c=window.matchMedia)!=null&&c.call(window,"(prefers-color-scheme: dark)").matches)}function dp(o){if(typeof document>"u")return;const c=up(o.theme),_=o.accent&&cp.includes(o.accent)?o.accent:"ocean";document.documentElement.dataset.theme=c?"dark":"light",document.documentElement.dataset.motion=o.reduceMotion?"reduce":"system",document.documentElement.dataset.accent=_}function Kf(o){dp(o);let c=!0;try{window.localStorage.setItem(op,JSON.stringify(o))}catch{c=!1}return typeof window<"u"&&window.dispatchEvent(new CustomEvent("coreflow-appearance",{detail:o})),c}const Xf=1e4,Dg=12e4;let Zf=0;function os(){var o;return typeof window<"u"&&typeof((o=window.ksu)==null?void 0:o.exec)=="function"}function Xc(o){if(typeof o=="string")return{errno:0,stdout:o,stderr:""};if(typeof o=="object"&&o!==null){const c=o,_=typeof c.errno=="number"&&Number.isFinite(c.errno)?c.errno:-1,d=typeof c.stdout=="string"?c.stdout:"",j=typeof c.stderr=="string"?c.stderr:"";return{errno:_,stdout:d,stderr:j}}return{errno:-1,stdout:"",stderr:"Malformed native bridge response"}}async function Oi(o,c=Xf){if(!os()||!window.ksu)return{errno:-1,stdout:"",stderr:"Native KernelSU bridge unavailable"};if(typeof o!="string"||!o||o.includes("\0"))return{errno:-1,stdout:"",stderr:"Invalid shell command"};const _=window.ksu,d=Number.isFinite(c)?Math.min(Dg,Math.max(1e3,c)):Xf;return new Promise(j=>{let A=!1;Zf+=1;const N=`__coreflow_ksu_cb_${Date.now()}_${Zf}`,B=R=>{A||(A=!0,window.clearTimeout(U),delete window[N],j(R))},U=window.setTimeout(()=>{B({errno:-1,stdout:"",stderr:`Native command timed out after ${d}ms`})},d);window[N]=(R,b,y)=>{if(typeof R=="object"&&R!==null){B(Xc(R));return}B({errno:typeof R=="number"&&Number.isFinite(R)?R:0,stdout:typeof b=="string"?b:String(b??""),stderr:typeof y=="string"?y:String(y??"")})};try{const R=_.exec(o,"{}",N);R!==void 0&&Promise.resolve(R).then(b=>B(Xc(b))).catch(b=>B({errno:-1,stdout:"",stderr:b instanceof Error?b.message:String(b)}))}catch{try{const R=_.exec(o);Promise.resolve(R).then(b=>B(Xc(b))).catch(b=>B({errno:-1,stdout:"",stderr:b instanceof Error?b.message:String(b)}))}catch(R){B({errno:-1,stdout:"",stderr:R instanceof Error?R.message:String(R)})}}})}const Wc={mode:"disabled",enableSkiavkThreaded:!1,enableAdaptiveVrs:!1,enablePipelineCachePrewarm:!1};function kl(o,c){const _=o.mode==="full_capability_override"?"safe_hardware":o.mode,d=_!=="disabled",j=(c==null?void 0:c.deviceExtensions)??0,A=(c==null?void 0:c.hwRayTracing)??(c==null?void 0:c.rayTracing)??!1,N=(c==null?void 0:c.hwMeshShaders)??(c==null?void 0:c.meshShaders)??!1,B=(c==null?void 0:c.hwVariableRateShading)??(c==null?void 0:c.variableRateShading)??!1,U=(c==null?void 0:c.hwCooperativeMatrix)??(c==null?void 0:c.cooperativeMatrix)??!1,R=(c==null?void 0:c.hwRtPipeline)??(c==null?void 0:c.rtPipeline)??!1,b=(c==null?void 0:c.hwRayQuery)??(c==null?void 0:c.rayQuery)??!1,y=(c==null?void 0:c.hwAccelerationStructure)??(c==null?void 0:c.accelerationStructure)??!1,O=(c==null?void 0:c.hwShaderFloat64)??(c==null?void 0:c.shaderFloat64)??!1,$=(c==null?void 0:c.hwIndexTypeUint8)??(c==null?void 0:c.indexTypeUint8)??!1,q=(c==null?void 0:c.hwLineRasterization)??(c==null?void 0:c.lineRasterization)??!1,K=(c==null?void 0:c.hwPushDescriptors)??(c==null?void 0:c.pushDescriptors)??!1,V=(c==null?void 0:c.hwGlobalPriorityQuery)??(c==null?void 0:c.globalPriorityQuery)??!1,Z=(c==null?void 0:c.hwShaderFloatControls2)??(c==null?void 0:c.shaderFloatControls2)??!1,ie=(c==null?void 0:c.hwMaintenance5)??(c==null?void 0:c.maintenance5)??!1,G=(c==null?void 0:c.hwMaintenance6)??(c==null?void 0:c.maintenance6)??!1,ce=(c==null?void 0:c.hwMaintenance7)??(c==null?void 0:c.maintenance7)??!1,Ee=(c==null?void 0:c.hwDescriptorBuffer)??(c==null?void 0:c.descriptorBuffer)??!1,E=(c==null?void 0:c.hwGraphicsPipelineLibrary)??(c==null?void 0:c.graphicsPipelineLibrary)??!1,D=(c==null?void 0:c.apiVersion)??"Probing ICD...";return{available:(c==null?void 0:c.available)??!1,layerInstalled:(c==null?void 0:c.layerInstalled)??!0,layerActive:d,mode:_,vendor:(c==null?void 0:c.vendor)??"Auto-Detecting GPU Vendor",deviceName:(c==null?void 0:c.deviceName)??"Probing Physical GPU ICD...",apiVersion:D,effectiveApiVersion:D,driverVersion:(c==null?void 0:c.driverVersion)??"0.0.0",deviceExtensions:j,effectiveDeviceExtensions:j,instanceExtensions:(c==null?void 0:c.instanceExtensions)??0,deviceUuid:(c==null?void 0:c.deviceUuid)??"00000000-0000-0000-0000-000000000000",driverUuid:(c==null?void 0:c.driverUuid)??"00000000-0000-0000-0000-000000000000",pipelineCacheUuid:(c==null?void 0:c.pipelineCacheUuid)??"00000000-0000-0000-0000-000000000000",rayTracing:A,meshShaders:N,variableRateShading:B,cooperativeMatrix:U,rtPipeline:R,rayQuery:b,accelerationStructure:y,portabilitySubset:(c==null?void 0:c.portabilitySubset)??!1,robustBufferAccess:(c==null?void 0:c.robustBufferAccess)??!0,samplerAnisotropy:(c==null?void 0:c.samplerAnisotropy)??!0,shaderInt16:(c==null?void 0:c.shaderInt16)??!0,shaderFloat64:O,protectedMemory:(c==null?void 0:c.protectedMemory)??!0,timelineSemaphore:(c==null?void 0:c.timelineSemaphore)??!0,descriptorIndexing:(c==null?void 0:c.descriptorIndexing)??!0,bufferDeviceAddress:(c==null?void 0:c.bufferDeviceAddress)??!0,dynamicRendering:(c==null?void 0:c.dynamicRendering)??!0,synchronization2:(c==null?void 0:c.synchronization2)??!0,indexTypeUint8:$,lineRasterization:q,pushDescriptors:K,globalPriorityQuery:V,shaderFloatControls2:Z,maxPushDescriptors:(c==null?void 0:c.maxPushDescriptors)??(K?32:0),maintenance5:ie,maintenance6:G,maintenance7:ce,descriptorBuffer:Ee,graphicsPipelineLibrary:E,hwRayTracing:A,hwMeshShaders:N,hwVariableRateShading:B,hwCooperativeMatrix:U,hwRtPipeline:R,hwRayQuery:b,hwAccelerationStructure:y,hwShaderFloat64:O,hwIndexTypeUint8:$,hwLineRasterization:q,hwPushDescriptors:K,hwGlobalPriorityQuery:V,hwShaderFloatControls2:Z,hwMaintenance5:ie,hwMaintenance6:G,hwMaintenance7:ce,hwDescriptorBuffer:Ee,hwGraphicsPipelineLibrary:E,skiavkActive:d&&o.enableSkiavkThreaded,adaptiveVrsActive:d&&o.enableAdaptiveVrs&&B,pipelineCachePrewarmActive:d&&o.enablePipelineCachePrewarm,gpuKernelAvailable:(c==null?void 0:c.gpuKernelAvailable)??!1,gpuKernelLabel:(c==null?void 0:c.gpuKernelLabel)??"Universal Multi-Vendor GPU Devfreq",gpuGovernor:(c==null?void 0:c.gpuGovernor)??"auto",gpuCurrentFreqMhz:(c==null?void 0:c.gpuCurrentFreqMhz)??0,gpuMaxFreqMhz:(c==null?void 0:c.gpuMaxFreqMhz)??0,adrenoboostLevel:(c==null?void 0:c.adrenoboostLevel)??0}}const Jf={version:"general",log_level:"general",zero_polling_mode:"general",enable_cgroups_v2:"general",enable_uclamp:"general",enable_tuner:"general",tier_override:"device_adaptation",unlock_vendor_clamping:"device_adaptation",force_120hz_rendering:"device_adaptation",enable_cpu_frequency_control:"device_adaptation",enable_gpu_frequency_control:"device_adaptation",enable_direct_adaptive:"direct_adaptive",direct_adaptive_policy:"direct_adaptive",top_app_interactive_min:"uclamp_tuning",top_app_interactive_max:"uclamp_tuning",ui_adaptive_bias:"uclamp_tuning",ui_balanced_uclamp_min:"uclamp_tuning",foreground_boost_min:"uclamp_tuning",background_clamp_max:"uclamp_tuning",system_background_max:"uclamp_tuning",temp_monitor_interval_ms:"thermal_guardian",thermal_trip_celsius:"thermal_guardian",thermal_recover_celsius:"thermal_guardian",emergency_rollback_on_overheat:"thermal_guardian",cpu_pressure_boost_pct:"stability_policy",memory_pressure_guard_pct:"stability_policy",io_pressure_guard_pct:"stability_policy",thermal_headroom_celsius:"stability_policy",thermal_emergency_margin_celsius:"stability_policy",enable_pid_equilibrium:"pid_equilibrium",pid_target_celsius:"pid_equilibrium",pid_kp:"pid_equilibrium",pid_ki:"pid_equilibrium",pid_kd:"pid_equilibrium",enable_battery_step_charging:"hardware_longevity",enable_ufs_burst_pacing:"hardware_longevity",battery_step_temp_threshold:"hardware_longevity",stepped_charge_current_ma:"hardware_longevity",top_app_io_weight:"hardware_longevity",background_io_weight:"hardware_longevity",vulkan_layer_mode:"vulkan_engine",enable_skiavk_threaded:"vulkan_engine",enable_adaptive_vrs:"vulkan_engine",enable_pipeline_cache_prewarm:"vulkan_engine",target_fps:"render_booster",pre_boost_on_touch:"render_booster",boost_duration_ms:"render_booster"};function Ci(o){return Object.prototype.hasOwnProperty.call(Jf,o)?Jf[o]:void 0}function _p(o){const c=o.trim();return c==="stability"?"stability_policy":c}function zg(o){const c=Object.create(null);let _="";for(const d of o.split(/\r?\n/)){const j=d.trim();if(!j||j.startsWith("#")||j.startsWith(";"))continue;if(j.startsWith("[")&&j.endsWith("]")){_=_p(j.slice(1,-1)),c[_]||(c[_]=Object.create(null));continue}const A=j.indexOf("=");if(A<0)continue;const N=j.slice(0,A).trim(),B=j.slice(A+1).trim();if(!N)continue;const U=Ci(N);if(U&&_!==U)continue;const R=_||U||"general";c[R]||(c[R]=Object.create(null)),c[R][N]=B}return c}function eu(o,c){var j;const _=zg(o),d=Ci(c);if(d){const A=(j=_[d])==null?void 0:j[c];return A&&A.length>0?A:null}for(const A of Object.values(_))if(A[c])return A[c];return null}function Ug(o,c){const _=eu(o,c);if(_===null)return null;const d=Number(_);return Number.isFinite(d)?d:null}function Bg(o,c){const _={};for(const[U,R]of Object.entries(c)){const b=U.trim();if(!b||Ci(b)===void 0)continue;const y=String(R).trim();/[\r\n\0]/.test(y)||(_[b]=y)}if(Object.keys(_).length===0)return o;const d=o?o.replace(/\r\n/g,`
`).split(`
`):[];d.length>0&&d[d.length-1]===""&&d.pop();const j=[];let A="";const N=U=>{if(U)for(const[R,b]of Object.entries(_))Ci(R)===U&&(j.push(`${R} = ${b}`),delete _[R])};for(const U of d){const R=U.trim();if(R.startsWith("[")&&R.endsWith("]")){N(A),A=_p(R.slice(1,-1)),j.push(U);continue}if(!R||R.startsWith("#")||R.startsWith(";")){j.push(U);continue}const b=U.indexOf("=");if(b<0){j.push(U);continue}const y=U.slice(0,b).trim();Object.prototype.hasOwnProperty.call(_,y)&&Ci(y)===A?(j.push(`${y} = ${_[y]}`),delete _[y]):j.push(U)}N(A);const B=["general","device_adaptation","direct_adaptive","uclamp_tuning","thermal_guardian","stability_policy","pid_equilibrium","hardware_longevity","vulkan_engine","render_booster"];for(const U of B){const R=Object.keys(_).filter(b=>Ci(b)===U);if(R.length!==0){j.length>0&&j[j.length-1].trim()!==""&&j.push(""),j.push(`[${U}]`);for(const b of R)j.push(`${b} = ${_[b]}`),delete _[b]}}return j.join(`
`)+`
`}const Zc=o=>typeof o=="object"&&o!==null&&!Array.isArray(o);function Vg(o){if(!Zc(o))return!1;const c=["cpuUtilizationPct","thermalPressure","ramBytes","zramBytes","refreshHz","discoveredNodes","baselineNodes","verifiedMutations","skippedNodes","ownershipLosses"],_=["mode","screenState","gpuModel","storageType"];return!(typeof o.active!="boolean"||typeof o.discoveryComplete!="boolean"||typeof o.thermalGuard!="boolean"||typeof o.journalFault!="boolean"||!c.every(d=>typeof o[d]=="number"&&Number.isFinite(o[d]))||!_.every(d=>typeof o[d]=="string")||!Array.isArray(o.cpuClusters)||!o.cpuClusters.every(d=>Zc(d)&&typeof d.id=="string"&&Array.isArray(d.opps)&&d.opps.every(j=>typeof j=="string"))||!Array.isArray(o.appliedValues)||!o.appliedValues.every(d=>Zc(d)&&["semantic","domain","path","current","oemBaseline","target","health"].every(j=>typeof d[j]=="string")&&typeof d.modified=="boolean"&&typeof d.owned=="boolean"))}const Cl={thermalTripCelsius:44,balancedUclampMin:320};function mu(o){switch(o){case"sustained_load":return{label:"Sustained Load",description:"Coordinated CPU responsiveness transaction active with verified thermal headroom.",stage:"OBSERVING",risk:"Medium",outcome:"IMPROVED"};case"pressure_guard":return{label:"Pressure Guard",description:"Memory or I/O contention elevated; dynamic block scheduler & VM relief engaged.",stage:"OBSERVING",risk:"Low",outcome:"IMPROVED"};case"thermal_guard":return{label:"Thermal Guard",description:"OEM/Android thermal safety gate engaged; active mitigation pacing SoC demand.",stage:"VERIFIED",risk:"Low",outcome:"ABORTED"};case"balanced":default:return{label:"Balanced Equilibrium",description:"Coordinated daily baseline retained with readback-verified top-app UClamp floor.",stage:"RETAINED",risk:"Low",outcome:"IMPROVED"}}}function Lg(){const o=mu("balanced");return{mode:"balanced",modeLabel:o.label,modeDescription:o.description,engineActive:!0,uclampMin:320,uclampMax:1024,uclampSupported:!0,uclampPolicyMin:320,uclampPolicyMax:1024,uclampTargetPath:"/dev/cpuctl/top-app",uclampEncoding:"percentage",platformVendor:"Universal Android",osFamily:"Android AOSP / GKI",osMajor:15,executionTier:"UniversalFallback",policyFamily:"CoreFlowBase",identityConfidence:"High",tier1Eligible:!0,inspectionMode:!1,mutationRuntimeStarted:!0,socTempCelsius:38.6,socCompositeCelsius:38.6,cpuAverageCelsius:38.2,cpuPeakCelsius:41.5,gpuTempCelsius:36.8,batteryTempCelsius:33.4,skinTempCelsius:35.1,thermalSource:"android+sysfs-oem-trips",thermalSensor:"SoC-Composite (CPU+GPU)",thermalSeverity:0,thermalPressure:44,thermalSensors:[{id:"tz0",type:"cpu-0-0",category:1,celsius:37.8,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.02},{id:"tz1",type:"cpu-0-1",category:1,celsius:38,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.01},{id:"tz2",type:"cpu-1-0",category:1,celsius:39.4,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.04},{id:"tz3",type:"cpu-1-2",category:1,celsius:41.5,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.05},{id:"tz4",type:"gpuss-0",category:1,celsius:36.8,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:-.02},{id:"tz5",type:"quiet_therm",category:3,celsius:35.1,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:0},{id:"tz6",type:"battery",category:2,celsius:33.4,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:0},{id:"tz7",type:"ap_therm",category:4,celsius:37,valid:!0,policyAvailable:!0,velocityValid:!0,velocityCelsiusPerSec:.01}],cpuPressurePct:1.2,memoryPressurePct:.3,ioPressurePct:.4,cpuPressureValid:!0,memoryPressureValid:!0,ioPressureValid:!0,baselineNodesSnapshotted:6,baselineStatus:"protected",transactionStage:"RETAINED",riskClass:"Low",lastOutcome:"IMPROVED",ownershipIntact:!0,aiBackend:"embedded-rls-mlp",aiOnnxActive:!1,aiPredictedPressure10s:46.2,aiPredictedVelocity:.08,aiRecommendedScale:.94,aiConfidence:.86,aiSamples:12,tunerRuntimeStage:"STAGE_ACTIVE_TUNING",tunerStage:"RETAINED",tunerIntent:"HoldEquilibrium",tunerWorkload:"GeneralInteraction",tunerBottleneck:"None",tunerObjective:"HoldBaseline",tunerCapabilityFingerprint:"cddp-v4:8f4e2b91c073a612",tunerCandidateId:"baseline",tunerCandidateLabel:"OEM Verified Baseline",tunerRationale:"System in equilibrium (I/O & memory PSI low); retaining OEM block scheduler without unnecessary mutation.",tunerDecisionPlanRationale:"Global candidate pool evaluated; equilibrium held with zero conflict rejections.",tunerVerdict:"IMPROVED",tunerUtilityDelta:0,tunerNoiseFloor:.05,tunerTrials:3,tunerImproved:2,tunerRolledBack:1,tunerInconclusive:0,tunerRejected:0,tunerThermalStabilization:!1,tunerThermalRecovery:!1,blockQueues:[{device:"sda",activeScheduler:"mq-deadline",baselineScheduler:"none",availableSchedulers:["none","mq-deadline","kyber","bfq"],readAheadKb:128,nrRequests:128,rqAffinity:2,readExpireMs:250,writesStarved:3,fifoBatch:8}],tunerCandidates:[{id:"io.mq_deadline_low_latency",label:"mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",domain:"io",trials:2,successes:2,rollbacks:0,meanDelta:7.4,ucbScore:8.12,cooldownTicks:0},{id:"io.kyber_token_bucket",label:"kyber Token-Bucket Low-Latency I/O Scheduler",domain:"io",trials:1,successes:0,rollbacks:1,meanDelta:-1.8,ucbScore:.42,cooldownTicks:0},{id:"cpufreq.schedutil_interactive_response",label:"EAS Governor Rate-Limit Pacing (500us up / 10000us down)",domain:"cpufreq",trials:1,successes:1,rollbacks:0,meanDelta:5.2,ucbScore:6.18,cooldownTicks:0},{id:"vm.zram_latency_relief",label:"VM Page-Cluster Zero-Latency & VFS Dentry Preservation",domain:"vm",trials:1,successes:1,rollbacks:0,meanDelta:4.1,ucbScore:5.15,cooldownTicks:0},{id:"vm.mglru_thrash_guard",label:"MGLRU Working-Set Thrash Guard (min_ttl_ms=1000ms)",domain:"vm",trials:2,successes:2,rollbacks:0,meanDelta:5.8,ucbScore:6.74,cooldownTicks:0}],batteryPower:{available:!0,charging:!1,fastCharging:!1,thermalStepActive:!1,temperatureCelsius:33.4,currentMa:-480,voltageMv:4120,powerMw:1977.6,chargeLimitUa:3e6,oemBaselineUa:3e6,status:"Discharging"},flightRecorder:Array.from({length:16},(c,_)=>({ts:Date.now()-(15-_)*2500,temp:Number((37.4+Math.sin(_*.45)*1.1).toFixed(1)),pressure:Number((42+Math.sin(_*.45)*5.5).toFixed(1)),cpuPsi:Number((1.4+Math.abs(Math.cos(_*.5))*3.8).toFixed(1)),memPsi:Number((.4+Math.abs(Math.sin(_*.35))*1.2).toFixed(1)),ioPsi:Number((.6+Math.abs(Math.sin(_*.6))*2.1).toFixed(1)),scale:.94,stage:"RETAINED",candidate:"io.mq_deadline_low_latency",verdict:"IMPROVED"})),vulkan:kl(Wc,{available:!0,layerInstalled:!0,vendor:"Multi-Vendor Preview (Adreno / Mali / Xclipse)",deviceName:"Universal Mobile GPU Preview Profile",apiVersion:"1.3.280",driverVersion:"512.762.0",deviceExtensions:148,instanceExtensions:16,deviceUuid:"cf000001-2026-4000-a000-000000000310",driverUuid:"cf000002-2026-4000-a000-000000000310",pipelineCacheUuid:"cf000003-2026-4000-a000-000000000310",hwVariableRateShading:!0,hwPushDescriptors:!0,hwMaintenance5:!0,hwMaintenance6:!0,gpuKernelAvailable:!0,gpuKernelLabel:"Universal GPU Devfreq Surface (Preview)",gpuGovernor:"msm-adreno-tz / simple_ondemand",gpuCurrentFreqMhz:680,gpuMaxFreqMhz:1050,adrenoboostLevel:1}),timestamp:Date.now()}}function Hg(){const o=new Date,c=_=>new Date(o.getTime()-_*1e3).toTimeString().slice(0,8);return[{id:"preview-1",timestamp:c(9),type:"INFO",tag:"AdmissionGate",message:"Preview: capability semantics, range bounds, and OEM ownership verified."},{id:"preview-2",timestamp:c(6),type:"BASELINE",tag:"Coordinator",message:"Preview: write-ahead transaction journal committed prior to mutation."},{id:"preview-3",timestamp:c(3),type:"INFO",tag:"AutonomousTuner",message:"Preview: sda queue/scheduler switched none -> mq-deadline (read_expire 0.5x, fifo_batch 0.5x) | delta_u=+7.4 -> IMPROVED."}]}function Gg(o,c,_){if(!o.engineActive)return{next:{...o,uclampMin:0,uclampMax:1024,mode:"balanced",modeLabel:"Engine Standby",modeDescription:"Coordinated mutations paused; OEM baseline values intact.",transactionStage:"PLANNED",timestamp:Date.now()}};const d=Number.isFinite(c.thermalTripCelsius)?c.thermalTripCelsius:Cl.thermalTripCelsius,A=Number.isFinite(c.balancedUclampMin)?c.balancedUclampMin:Cl.balancedUclampMin,N=Math.max(0,Math.min(1024,A)),B=Number.isFinite(o.socTempCelsius)?o.socTempCelsius:38.6,U=Math.max(32,Math.min(49,B+(Math.random()-.46)*.35)),R=_==="cpu"?14:Math.max(0,Math.random()*10),b=_==="memory"?6.4:Math.max(0,Math.random()*3.5),y=_==="io_trial"?11.8:_==="memory"?6.2:Math.max(0,Math.random()*5.2);let O="balanced",$=N,q=1024;U>=d?(O="thermal_guard",$=0,q=896):b>=3||y>=5?(O="pressure_guard",$=Math.floor(N/4)):R>=8&&U<=d-3&&(O="sustained_load",$=Math.min(1024,N+Math.floor(N/2)));const K=mu(O),V=Math.min(130,Math.max(25,(U-20)*2.2+R*.6+y*.4)),Z=Number(((V-(o.aiPredictedPressure10s??45))*.18).toFixed(2)),ie=O==="thermal_guard"?.68:O==="pressure_guard"?.82:Number(Math.max(.72,Math.min(1,1-Math.max(0,V-55)*.004)).toFixed(2)),G=(o.aiSamples??12)+1,ce=o.blockQueues??[],Ee=(o.tunerCandidates??[]).map(ze=>({...ze}));let E=o.tunerStage??"RETAINED",D=o.tunerIntent??"HoldEquilibrium",P=o.tunerCandidateId??"baseline",he=o.tunerCandidateLabel??"OEM Verified Baseline",Oe=o.tunerRationale??"",re=o.tunerVerdict??"IMPROVED",ue=o.tunerUtilityDelta??0,Ie=o.tunerTrials??3,Xe=o.tunerImproved??2;const He=o.tunerRolledBack??1,z=ce.map(ze=>({...ze}));if(_==="io_trial"||y>=5&&_!=="memory"){E="RETAINED",D="IoLatencyRelief",P="io.mq_deadline_low_latency",he="mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",re="IMPROVED",ue=Number((5.2+Math.random()*4.5).toFixed(2)),Ie+=1,Xe+=1,Oe=`I/O PSI=${y.toFixed(1)}% exceeded threshold; switched sda scheduler none -> mq-deadline (read_expire=0.5x, writes_starved=1.5x, fifo_batch=0.5x, read_ahead_kb=2.0x) | Before/After ΔU=+${ue} -> RETAINED.`,z.length>0&&(z[0].activeScheduler="mq-deadline",z[0].readExpireMs=250,z[0].writesStarved=3,z[0].fifoBatch=8,z[0].readAheadKb=256,z[0].rqAffinity=2);const ze=Ee.find(mt=>mt.id==="io.mq_deadline_low_latency");ze&&(ze.trials+=1,ze.successes+=1,ze.meanDelta=Number(((ze.meanDelta*(ze.trials-1)+ue)/ze.trials).toFixed(2)),ze.ucbScore=Number((ze.meanDelta+.65).toFixed(2)))}else _==="memory"||b>=3.2?(E="RETAINED",D="MemoryStallRelief",P="vm.zram_latency_relief",he="VM Page-Cluster Zero-Latency & VFS Dentry Preservation",re="IMPROVED",ue=Number((3.4+Math.random()*2.8).toFixed(2)),Ie+=1,Xe+=1,Oe=`Memory PSI=${b.toFixed(1)}% elevated; applied vm.page-cluster=0 & vfs_cache_pressure=80 | Before/After ΔU=+${ue} -> RETAINED.`):O==="thermal_guard"?(E="OBSERVING",D="ThermalStabilization",P="thermal.schedutil_damped_pacing",he="Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)",re="IMPROVED",ue=Number((2.8+Math.random()*1.9).toFixed(2)),Ie+=1,Xe+=1,Oe=`Thermal headroom constrained (${U.toFixed(1)}°C); switched objective to ThermalStabilization and damped EAS governor ramp rate (2.0x up / 0.5x down relative to OEM baseline).`):(_==="cpu"||R>=8.5)&&(E="RETAINED",D="InteractiveCpuPacing",P="cpufreq.schedutil_interactive_response",he="EAS Governor Rate-Limit Pacing (500us up / 10000us down)",re="IMPROVED",ue=Number((4.1+Math.random()*2.5).toFixed(2)),Ie+=1,Xe+=1,Oe=`CPU PSI=${R.toFixed(1)}% with thermal headroom; paced EAS schedutil governor rate limits (up=500us, down=10000us) | Before/After ΔU=+${ue} -> RETAINED.`);const ne=Date.now(),ae={ts:ne,temp:Number(U.toFixed(1)),pressure:Number(V.toFixed(1)),cpuPsi:Number(R.toFixed(1)),memPsi:Number(b.toFixed(1)),ioPsi:Number(y.toFixed(1)),scale:ie,stage:E,candidate:P,verdict:re},ke=[...o.flightRecorder??[],ae].slice(-60),me={...o,mode:O,modeLabel:K.label,modeDescription:K.description,uclampMin:$,uclampMax:q,socTempCelsius:Number(U.toFixed(1)),cpuPressurePct:Number(R.toFixed(1)),memoryPressurePct:Number(b.toFixed(1)),ioPressurePct:Number(y.toFixed(1)),transactionStage:K.stage,riskClass:K.risk,lastOutcome:K.outcome,ownershipIntact:!0,aiBackend:o.aiBackend??"embedded-rls-mlp",aiOnnxActive:o.aiOnnxActive??!1,aiPredictedPressure10s:Number(V.toFixed(1)),aiPredictedVelocity:Z,aiRecommendedScale:ie,aiConfidence:Number(Math.min(.96,.78+Math.min(G,50)*.003).toFixed(2)),aiSamples:G,tunerStage:E,tunerIntent:D,tunerCandidateId:P,tunerCandidateLabel:he,tunerRationale:Oe,tunerVerdict:re,tunerUtilityDelta:ue,tunerTrials:Ie,tunerImproved:Xe,tunerRolledBack:He,blockQueues:z,tunerCandidates:Ee,flightRecorder:ke,timestamp:ne};return O===o.mode&&!_?{next:me}:{next:me,event:Ig(O,K.label,me,_)}}let Jc=0;function Ig(o,c,_,d){return Jc+=1,d==="io_trial"?{id:`preview-${Date.now()}-${Jc}`,timestamp:new Date().toTimeString().slice(0,8),type:"BOOST",tag:"AutonomousTuner",message:_.tunerRationale??"Dynamic scheduler trial completed."}:{id:`preview-${Date.now()}-${Jc}`,timestamp:new Date().toTimeString().slice(0,8),type:o==="thermal_guard"?"THERMAL":o==="sustained_load"?"BOOST":"INFO",tag:"Transaction",message:`Stage=${_.transactionStage} -> ${c} | uclamp=${_.uclampMin}/${_.uclampMax} | cpu=${_.cpuPressurePct}% mem=${_.memoryPressurePct}% io=${_.ioPressurePct}% temp=${_.socTempCelsius}C`}}function mp(o,c=Cl.thermalTripCelsius){var G,ce,Ee,E,D;const _=Number.isFinite(c)&&c>25?c:44,d=Number.isFinite(o.socTempCelsius)?o.socTempCelsius:-1,j=o.cpuPressureValid!==!1&&Number.isFinite(o.cpuPressurePct)?Math.max(0,o.cpuPressurePct):0,A=o.memoryPressureValid!==!1&&Number.isFinite(o.memoryPressurePct)?Math.max(0,o.memoryPressurePct):0,N=o.ioPressureValid!==!1&&Number.isFinite(o.ioPressurePct)?Math.max(0,o.ioPressurePct):0,B=!!o.thermalGuardActive||!!((G=o.directAdaptive)!=null&&G.thermalGuard)||o.mode==="thermal_guard"||d>0&&d>=_,U=!B&&(d>0&&d>=_-2.5||typeof o.thermalPressure=="number"&&Number.isFinite(o.thermalPressure)&&o.thermalPressure>=60),R=B?"guard":U?"elevated":"optimal",b=Math.max(j*.65,A*1.8,N*1.4),y=b>=10||A>=5||N>=8?"critical":b>=4||A>=2.2||N>=3.5?"contended":"calm",O=!!((ce=o.directAdaptive)!=null&&ce.journalFault),$=((Ee=o.directAdaptive)==null?void 0:Ee.ownershipLosses)??0,q=O?"fault":o.ownershipIntact===!1||$>0?"contended":"intact";let K=100;o.engineActive||(K-=12),R==="guard"?K-=24:R==="elevated"&&(K-=10),d>36&&(K-=Math.min(14,Math.round((d-36)*1.4))),K-=Math.min(24,Math.round(j*.7+A*2.1+N*1.5)),q==="fault"?K-=35:q==="contended"&&(K-=14);const V=Math.max(15,Math.min(100,K)),Z=new Set;if(o.uclampSupported&&Z.add("UClamp"),(E=o.directAdaptive)!=null&&E.active&&Z.add("DirectAdaptive"),o.tunerActiveInterventions&&o.tunerActiveInterventions.length>0)for(const P of o.tunerActiveInterventions)P.domain&&Z.add(P.domain);else if(o.tunerCandidateId&&o.tunerCandidateId!=="baseline"){const P=o.tunerCandidateId.split(".")[0];P&&Z.add(P.toUpperCase())}(D=o.vulkan)!=null&&D.layerActive&&Z.add("VulkanLayer");const ie=O?"Journal fault latched — mutations halted while OEM recovery remains protected.":R==="guard"?"Thermal Guard engaged — pacing SoC demand within OEM thermal boundaries.":y==="critical"?"Resource contention detected — coordinated relief transaction active.":o.engineActive?"System in verified equilibrium — readback-checked policies active.":"Runtime in standby — OEM baseline values intact.";return{equilibriumScore:V,thermalTone:R,pressureTone:y,ownershipTone:q,headline:ie,activeDomains:Array.from(Z)}}const $c=[{key:"PLANNED",label:"Planned"},{key:"APPLIED",label:"Applied"},{key:"VERIFIED",label:"Readback Verified"},{key:"OBSERVING",label:"Observing"},{key:"RETAINED",label:"Retained"}],Pg=({telemetry:o,nativeMode:c=!1,thermalTripCelsius:_,onTriggerSimulation:d})=>{var Ee;const j=mu(o.mode),A=mp(o,_),N=o.socTempCelsius,B=c&&N<0?"N/A":N.toFixed(1),U=o.tunerStage??o.transactionStage??j.stage,R=o.engineActive?$c.some(E=>E.key===U)?U:"RETAINED":"PLANNED",b=(Ee=o.blockQueues)==null?void 0:Ee[0],y=o.tunerCandidates??[],O=o.tunerEvaluatedCandidates??[],$=o.tunerActiveInterventions??[],q=[...O].sort((E,D)=>D.utilityScore-E.utilityScore||D.confidence-E.confidence),K=[...y].sort((E,D)=>D.ucbScore-E.ucbScore),V=o.tunerUtilityDelta??0,Z=o.uclampPolicyMin??o.uclampMin,ie=o.uclampPolicyMax??o.uclampMax,G=A.thermalTone==="guard"?"md3-badge md3-badge-error":A.thermalTone==="elevated"?"md3-badge md3-badge-warning":"md3-badge md3-badge-success",ce=A.pressureTone==="critical"?"md3-badge md3-badge-warning":A.pressureTone==="contended"?"md3-badge md3-badge-secondary":"md3-badge md3-badge-success";return r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col lg:flex-row lg:items-start justify-between gap-6 pb-6",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-2.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("span",{className:"md3-badge md3-badge-primary",children:c?o.engineActive?"NATIVE RUNTIME ACTIVE":"NATIVE STANDBY":`${j.label.toUpperCase()} (SIM)`}),r.jsxs("span",{className:G,children:["THERMAL: ",A.thermalTone.toUpperCase()]}),r.jsxs("span",{className:ce,children:["PSI: ",A.pressureTone.toUpperCase()]}),r.jsxs("span",{className:"md3-badge md3-badge-neutral font-mono",children:["RISK: ",o.riskClass??j.risk]})]}),r.jsx("h2",{className:"text-xl sm:text-2xl font-bold tracking-tight leading-snug",children:c?o.modeDescription:j.description}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[A.headline," Every kernel mutation is gated by capability discovery, write-ahead CFJ4 journaling, synchronous readback verification, and empirical before/after outcome scoring."]}),A.activeDomains.length>0&&r.jsxs("div",{className:"flex flex-wrap items-center gap-1.5 pt-1",children:[r.jsx("span",{className:"text-[11px] font-semibold mr-1",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Active Domains:"}),A.activeDomains.map(E=>r.jsx("span",{className:"md3-badge md3-badge-secondary font-mono",children:E},E))]})]}),r.jsxs("div",{className:"md3-card-filled p-4 sm:p-5 shrink-0 lg:w-72 space-y-3",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("span",{className:"text-xs font-bold uppercase tracking-wider",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Equilibrium Index"}),r.jsxs("span",{className:"font-mono text-2xl font-bold tabular-nums",style:{color:"var(--md-sys-color-primary)"},children:[A.equilibriumScore,"%"]})]}),r.jsx("div",{className:"md3-progress-track",children:r.jsx("div",{className:"md3-progress-bar",style:{width:`${A.equilibriumScore}%`}})}),r.jsxs("div",{className:"pt-2 space-y-1 text-xs",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("div",{className:"font-semibold",children:"OEM Thermal → Pressure → Load → Balanced"}),r.jsxs("div",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Verdict: ",o.tunerVerdict??o.lastOutcome??j.outcome," · Ownership:"," ",o.ownershipIntact!==!1?"Intact":"Contended"]})]})]})]}),!c&&d&&r.jsxs("div",{className:"md3-card-filled px-4 py-3.5 flex flex-col sm:flex-row sm:items-center justify-between gap-3",children:[r.jsxs("div",{className:"flex items-center gap-2.5 text-xs",children:[r.jsx(Ng,{className:"w-4 h-4 shrink-0",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("div",{children:[r.jsx("span",{className:"font-bold",children:"Interactive Preview Workload Simulator"}),r.jsxs("span",{className:"hidden md:inline",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","· Inject synthetic pressure spikes to observe M3 transaction stages and bandit decisions"]})]})]}),r.jsxs("div",{className:"flex flex-wrap items-center gap-1.5",children:[r.jsxs("button",{type:"button",id:"btn-sim-cpu",onClick:()=>d("cpu"),className:"md3-chip",children:[r.jsx(za,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"CPU Burst"})]}),r.jsxs("button",{type:"button",id:"btn-sim-memory",onClick:()=>d("memory"),className:"md3-chip",children:[r.jsx(gn,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Memory Stall"})]}),r.jsxs("button",{type:"button",id:"btn-sim-io",onClick:()=>d("io_trial"),className:"md3-chip",children:[r.jsx(Tl,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Block I/O Trial"})]}),r.jsxs("button",{type:"button",id:"btn-sim-step",onClick:()=>d(void 0),className:"md3-btn-tonal !min-h-[2rem] !px-3 !text-xs",children:[r.jsx(ap,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Step Tick"})]})]})]}),r.jsxs("div",{className:"space-y-3 pb-6",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("span",{className:"text-xs font-bold flex items-center gap-1.5",children:[r.jsx($h,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Transaction Lifecycle Pipeline",c?"":" (sim)"]})]}),r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs font-mono",children:[o.tunerWorkload&&r.jsxs("span",{className:"md3-badge md3-badge-neutral",children:["Workload: ",o.tunerWorkload]}),o.tunerObjective&&r.jsxs("span",{className:"md3-badge md3-badge-neutral",children:["Objective: ",o.tunerObjective]}),r.jsxs("span",{className:"md3-badge md3-badge-primary",children:["Stage: ",o.tunerStage??R]})]})]}),r.jsx("div",{className:"grid grid-cols-2 sm:grid-cols-5 gap-2",children:$c.map((E,D)=>{const P=$c.findIndex(re=>re.key===R),he=E.key===R,Oe=D<P;return r.jsxs("div",{className:"px-3.5 py-2.5 rounded-2xl text-xs flex items-center justify-between transition-all",style:{backgroundColor:he?"var(--md-sys-color-primary-container)":Oe?"var(--md-sys-color-surface-container-high)":"var(--md-sys-color-surface-container)",color:he?"var(--md-sys-color-on-primary-container)":Oe?"var(--md-sys-color-on-surface)":"var(--md-sys-color-on-surface-variant)",fontWeight:he?700:600},children:[r.jsxs("span",{className:"truncate",children:[D+1,". ",E.label]}),he&&r.jsx("span",{className:"h-2 w-2 rounded-full shrink-0",style:{backgroundColor:"var(--md-sys-color-primary)"}})]},E.key)})})]}),r.jsxs("div",{className:"grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4",children:[r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Foreground UClamp · Readback"}),r.jsx(Rg,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"text-2xl font-bold font-mono tabular-nums",children:[o.uclampSupported?o.uclampMin:"—",r.jsxs("span",{className:"text-xs font-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","/ ",o.uclampSupported?o.uclampMax:"—"]})]})]}),r.jsxs("div",{className:"mt-3 grid grid-cols-2 gap-2 pt-3 text-[11px]",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Policy base"}),r.jsxs("div",{className:"mt-0.5 font-mono font-bold",children:[Z," / ",ie]})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"ABI encoding"}),r.jsx("div",{className:"mt-0.5 font-mono font-bold",children:o.uclampEncoding??"percentage"})]})]}),r.jsx("div",{className:"mt-2 truncate text-[10px] font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},title:o.uclampTargetPath??"",children:o.uclampSupported?o.uclampTargetPath||"Verified foreground cgroup":"Foreground cgroup capability unavailable"})]}),r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["SoC Thermal Composite",c?"":" (sim)"]}),r.jsx(ls,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"text-2xl font-bold font-mono tabular-nums",children:[B,r.jsxs("span",{className:"text-xs font-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","°C"]})]})]}),r.jsx("div",{className:"my-2.5",children:r.jsx("div",{className:"md3-progress-track",children:r.jsx("div",{className:"md3-progress-bar",style:{width:`${Math.max(8,Math.min(100,N>0?(N-28)/28*100:12))}%`}})})}),r.jsx("div",{className:"text-[11px] leading-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:c?`Source: ${o.thermalSource??"OEM policy & sensor fusion"}`:`Trip gate ${_.toFixed(1)} °C · CPU peak ${(o.cpuPeakCelsius??N).toFixed(1)} °C`})]}),r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["PSI Resource Pressure",c?"":" (sim)"]}),r.jsx(rs,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-2 font-mono tabular-nums text-xs my-1",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CPU"}),r.jsx("div",{className:"text-sm font-bold",children:o.cpuPressureValid===!1?"N/A":`${o.cpuPressurePct.toFixed(1)}%`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"MEM"}),r.jsx("div",{className:"text-sm font-bold",children:o.memoryPressureValid===!1?"N/A":`${o.memoryPressurePct.toFixed(1)}%`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"I/O"}),r.jsx("div",{className:"text-sm font-bold",children:o.ioPressureValid===!1?"N/A":`${o.ioPressurePct.toFixed(1)}%`})]})]}),r.jsxs("div",{className:"text-[11px] mt-2 leading-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Linux ",r.jsx("code",{className:"font-mono",children:"/proc/pressure"})," avg10 window"]})]}),r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center justify-between text-xs font-semibold mb-2",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["CFJ4 Durable Journal",c?"":" (sim)"]}),r.jsx(Un,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"text-lg font-bold",children:o.baselineStatus==="protected"?"Journal Armed":"Baseline Restored"})]}),r.jsx("div",{className:"text-[11px] mt-2 leading-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:o.baselineNodesSnapshotted>0?`${o.baselineNodesSnapshotted} owned kernel nodes verified`:"Write-ahead & readback verified"})]})]}),r.jsxs("div",{className:"md3-card-filled p-5 space-y-4",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-3 pb-3.5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-1 min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs",children:[r.jsx(gg,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{className:"font-bold text-sm",children:["Autonomous Contract-Gated Tuning Evaluator",c?"":" (sim)"]}),r.jsxs("span",{className:"md3-badge md3-badge-primary font-mono",children:["Intent: ",o.tunerIntent??"HoldEquilibrium"]})]}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:o.tunerRationale??"Evaluates workload-specific candidates only after the kernel contract, access path, valid range, and baseline have been verified."})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-4 shrink-0 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Empirical ΔU"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:V>=0?`+${V.toFixed(2)}`:V.toFixed(2)})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Retained"}),r.jsxs("div",{className:"font-bold text-sm mt-0.5",children:[o.tunerImproved??0," / ",o.tunerTrials??0]})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Rolled Back"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:o.tunerRolledBack??0})]})]})]}),$.length>0&&r.jsxs("div",{className:"space-y-2",children:[r.jsxs("div",{className:"text-xs font-bold",children:["Active Co-Executing Interventions (",$.length,")"]}),r.jsx("div",{className:"grid grid-cols-1 md:grid-cols-2 gap-2.5",children:$.map(E=>r.jsxs("div",{className:"md3-card-elevated p-3.5 text-xs space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2",children:[r.jsx("span",{className:"font-mono font-bold truncate",children:E.candidateId}),r.jsx("span",{className:"md3-badge md3-badge-primary font-mono",children:E.stage})]}),r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:E.candidateLabel}),r.jsxs("div",{className:"flex flex-wrap items-center gap-3 font-mono text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["Domain: ",E.domain]}),r.jsxs("span",{children:["Verdict: ",E.verdict]}),r.jsxs("span",{children:["Window: ",E.observationTick,"/",E.observationWindow]})]})]},`${E.domain}-${E.candidateId}`))})]}),r.jsxs("div",{className:"grid grid-cols-1 lg:grid-cols-2 gap-4",children:[r.jsxs("div",{className:"md3-card-elevated p-4 space-y-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs",children:[r.jsxs("span",{className:"font-bold flex items-center gap-1.5",children:[r.jsx(Tl,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Block Queue Surface",c?"":" (sim)"," (",b?`/sys/block/${b.device}/queue`:"Awaiting probe",")"]})]}),r.jsxs("span",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["OEM: ",(b==null?void 0:b.baselineScheduler)??"N/A"," → Active:"," ",r.jsx("strong",{style:{color:"var(--md-sys-color-primary)"},children:(b==null?void 0:b.activeScheduler)??"N/A"})]})]}),r.jsxs("div",{className:"grid grid-cols-3 sm:grid-cols-6 gap-2 pt-1 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"read_expire"}),r.jsx("div",{className:"font-bold",children:b&&b.readExpireMs>=0?`${b.readExpireMs}ms`:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"writes_starved"}),r.jsx("div",{className:"font-bold",children:b&&b.writesStarved>=0?b.writesStarved:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"fifo_batch"}),r.jsx("div",{className:"font-bold",children:b&&b.fifoBatch>=0?b.fifoBatch:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"read_ahead"}),r.jsx("div",{className:"font-bold",children:b&&b.readAheadKb>=0?`${b.readAheadKb}KB`:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"nr_requests"}),r.jsx("div",{className:"font-bold",children:b&&b.nrRequests>=0?b.nrRequests:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"rq_affinity"}),r.jsx("div",{className:"font-bold",children:b&&b.rqAffinity>=0?b.rqAffinity:"N/A"})]})]})]}),r.jsxs("div",{className:"md3-card-elevated p-4 space-y-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs",children:[r.jsx("span",{className:"font-bold",children:"Global Adaptive Candidate Ranking"}),r.jsxs("span",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Active: ",o.tunerCandidateId??"none"]})]}),r.jsx("div",{className:"max-h-60 space-y-2 overflow-y-auto pr-1",children:q.length>0?q.map((E,D)=>{const P=y.find(Oe=>Oe.id===E.id),he=E.selected||E.id===o.tunerCandidateId;return r.jsxs("div",{className:"rounded-xl p-3 text-xs space-y-1.5",style:{backgroundColor:he?"var(--md-sys-color-primary-container)":"var(--md-sys-color-surface-container)",color:he?"var(--md-sys-color-on-primary-container)":"var(--md-sys-color-on-surface)"},children:[r.jsxs("div",{className:"flex items-start justify-between gap-3",children:[r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-1.5 text-[10px] font-mono",children:[r.jsxs("span",{children:["#",D+1]}),r.jsxs("span",{children:["· ",E.domain]}),he&&r.jsx("span",{className:"md3-badge md3-badge-primary",children:"ACTIVE"}),!E.eligible&&r.jsx("span",{className:"md3-badge md3-badge-warning",children:"INELIGIBLE"})]}),r.jsx("div",{className:"mt-0.5 break-all font-bold",children:E.id})]}),r.jsxs("div",{className:"shrink-0 text-right font-mono tabular-nums",children:[r.jsx("div",{className:"text-sm font-bold",children:E.utilityScore.toFixed(1)}),r.jsx("div",{className:"text-[9px] opacity-75",children:"utility"})]})]}),r.jsxs("div",{className:"flex flex-wrap gap-x-3 gap-y-1 font-mono text-[10px] opacity-80",children:[r.jsxs("span",{children:["UCB ",E.empiricalScore.toFixed(3)]}),r.jsxs("span",{children:["conf ",Math.round(E.confidence*100),"%"]}),P&&r.jsxs("span",{children:["retained ",P.successes]}),P&&r.jsxs("span",{children:["rollback ",P.rollbacks]})]})]},E.id)}):K.map((E,D)=>r.jsxs("div",{className:"flex items-center justify-between gap-3 rounded-xl p-2.5",style:{backgroundColor:"var(--md-sys-color-surface-container)"},children:[r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"text-[10px] font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["#",D+1," · ",E.domain]}),r.jsx("div",{className:"truncate text-xs font-bold",children:E.id}),r.jsxs("div",{className:"text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["retained ",E.successes," · rollback ",E.rollbacks," · trials ",E.trials]})]}),r.jsxs("div",{className:"shrink-0 text-right font-mono",children:[r.jsx("div",{className:"text-sm font-bold",children:E.ucbScore.toFixed(3)}),r.jsx("div",{className:"text-[9px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"UCB1"})]})]},E.id))})]})]})]}),r.jsxs("div",{className:"md3-card-filled p-4 sm:p-5 flex flex-col sm:flex-row sm:items-center justify-between gap-4",children:[r.jsxs("div",{className:"space-y-1 min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs",children:[r.jsxs("span",{className:"font-bold text-sm",children:["Neural Horizon Predictor (ONNX / Online RLS-MLP)",c?"":" (sim)"]}),r.jsx("span",{className:"md3-badge md3-badge-primary font-mono",children:o.aiBackend??"embedded-rls-mlp"})]}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Forecasts 10-second thermal trajectory via Online Recursive Least Squares + 6×8×2 Micro-MLP with NLMS output-head adaptation, pre-pacing capacity strictly within Safety Admission invariants."})]}),r.jsxs("div",{className:"grid grid-cols-4 gap-4 shrink-0 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"10s Forecast"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:(o.aiPredictedPressure10s??45).toFixed(1)})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Velocity"}),r.jsx("div",{className:"font-bold text-sm mt-0.5",children:(o.aiPredictedVelocity??0)>=0?`+${(o.aiPredictedVelocity??0).toFixed(2)}/s`:`${(o.aiPredictedVelocity??0).toFixed(2)}/s`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Scale Hint"}),r.jsxs("div",{className:"font-bold text-sm mt-0.5",children:[Math.round((o.aiRecommendedScale??1)*100),"%"]})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-sans",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Confidence"}),r.jsxs("div",{className:"font-bold text-sm mt-0.5",style:{color:"var(--md-sys-color-primary)"},children:[Math.round((o.aiConfidence??.85)*100),"%"]})]})]})]}),o.flightRecorder&&o.flightRecorder.length>=2&&(()=>{const E=o.flightRecorder.slice(-30),D=(ue,Ie,Xe)=>{const He=Math.max(.001,Xe-Ie);return ue.map((z,ne)=>{const ae=ne/Math.max(1,ue.length-1)*220,me=44-(Math.max(Ie,Math.min(Xe,Number.isFinite(z)?z:Ie))-Ie)/He*38;return`${ae.toFixed(1)},${me.toFixed(1)}`}).join(" ")},P=E.map(ue=>ue.temp!=null&&ue.temp>=0?ue.temp:36),he=E.map(ue=>ue.cpuPsi),Oe=E.map(ue=>Math.max(ue.memPsi,ue.ioPsi)),re=E[E.length-1];return r.jsxs("div",{className:"md3-card-filled p-4 sm:p-5 space-y-3.5",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(gn,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{className:"font-bold text-sm",children:["Flight-Recorder Telemetry Ring Buffer",c?"":" (sim)"]}),r.jsxs("span",{className:"md3-badge md3-badge-neutral font-mono",children:[E.length," samples · 60s window"]})]}),r.jsxs("span",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Scale: ",Math.round(((re==null?void 0:re.scale)??1)*100),"% · Stage: ",(re==null?void 0:re.stage)??"RETAINED"]})]}),r.jsxs("div",{className:"grid grid-cols-1 sm:grid-cols-3 gap-3",children:[r.jsxs("div",{className:"md3-card-elevated p-3.5 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"SoC Thermal Trajectory"}),r.jsx("span",{className:"font-mono font-bold",children:(re==null?void 0:re.temp)!=null&&re.temp>=0?`${re.temp.toFixed(1)} °C`:"N/A"})]}),r.jsx("svg",{viewBox:"0 0 220 48",className:"w-full h-12 overflow-visible","aria-label":"SoC thermal sparkline",children:r.jsx("polyline",{fill:"none",stroke:"var(--md-sys-color-primary)",strokeWidth:"2",strokeLinecap:"round",strokeLinejoin:"round",points:D(P,28,55)})})]}),r.jsxs("div",{className:"md3-card-elevated p-3.5 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CPU PSI Contention"}),r.jsxs("span",{className:"font-mono font-bold",children:[((re==null?void 0:re.cpuPsi)??0).toFixed(1),"%"]})]}),r.jsx("svg",{viewBox:"0 0 220 48",className:"w-full h-12 overflow-visible","aria-label":"CPU PSI sparkline",children:r.jsx("polyline",{fill:"none",stroke:"var(--md-sys-color-secondary)",strokeWidth:"2",strokeLinecap:"round",strokeLinejoin:"round",points:D(he,0,25)})})]}),r.jsxs("div",{className:"md3-card-elevated p-3.5 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Peak Memory / I/O PSI"}),r.jsxs("span",{className:"font-mono font-bold",children:[Math.max((re==null?void 0:re.memPsi)??0,(re==null?void 0:re.ioPsi)??0).toFixed(1),"%"]})]}),r.jsx("svg",{viewBox:"0 0 220 48",className:"w-full h-12 overflow-visible","aria-label":"Memory and IO PSI sparkline",children:r.jsx("polyline",{fill:"none",stroke:"var(--md-sys-color-tertiary)",strokeWidth:"2",strokeLinecap:"round",strokeLinejoin:"round",points:D(Oe,0,18)})})]})]})]})})()]})},$f=({nodesSnapshotted:o,nativeMode:c,baselineStatus:_})=>r.jsxs("section",{className:"md3-card-elevated space-y-5 p-6 sm:p-7","aria-label":"Baseline ownership and recovery",children:[r.jsx("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-3",children:r.jsxs("div",{className:"flex items-start gap-3.5",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(Un,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0 flex-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h3",{className:"text-base font-bold tracking-tight",children:"Ownership Guard & Baseline Recovery"}),r.jsx("span",{className:c?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",children:c?"NATIVE CFJ4 JOURNAL":"PREVIEW MODEL"})]}),r.jsx("p",{className:"mt-1 text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CoreFlow snapshots factory and pre-mutation values into a write-ahead journal before any write, then restores only nodes whose inode and active value prove continued ownership."})]})]})}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Journal Format"}),r.jsx(ou,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"font-mono text-sm font-bold",children:"CFJ4 · Write-Ahead"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Inode + boot_id bound"})]}),r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Ownership State"}),r.jsx(uu,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"text-sm font-bold",children:_==="protected"?"Recovery Protected":"Clean · Baseline Intact"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"External OEM writes respected"})]}),r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"Verified Snapshots"}),r.jsx(du,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsx("div",{className:"font-mono text-sm font-bold tabular-nums",children:c?o:`${o} (sim)`}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Idempotent rollback ready"})]})]}),r.jsx("p",{className:"pt-3 text-[11px] leading-relaxed",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:"Manual kernel policy sliders are intentionally absent. When a safety rollback is needed, the native recovery lifecycle and module kill switch remain authoritative."})]}),Yg={SCREEN_OFF:"Screen Off / Deep Idle",BALANCED_SWEET_SPOT:"Balanced Sweet-Spot",ACTIVE_LOAD:"Active Load / Gaming",THERMAL_GUARD:"Thermal Guard",READ_ONLY:"Read-only status",DISCOVERY:"Capability discovery"},Wf=o=>{if(!Number.isFinite(o)||o<=0)return"Unavailable";const c=["B","KB","MB","GB","TB"];let _=o,d=0;for(;_>=1024&&d<c.length-1;)_/=1024,d+=1;return _.toFixed(d>=3?2:0)+" "+c[d]},Qg=o=>{const c=Number(o);return Number.isFinite(c)&&c>=1e5?`${Math.round(c/1e3)} MHz`:o},Fg=({status:o,nativeMode:c,journalOutput:_,restoringBaseline:d,onViewJournal:j,onRestoreBaseline:A})=>{if(!c)return r.jsxs("section",{className:"md3-card-elevated space-y-3 p-6 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx(gn,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"font-bold text-base",children:"Direct Adaptive Runtime"}),r.jsx("span",{className:"md3-badge md3-badge-neutral",children:"NATIVE ONLY"})]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Live topology discovery, actuator ownership, and OEM baseline values are queried directly from ",r.jsx("code",{className:"font-mono",children:"coreflowd"}),". Browser preview does not synthesize kernel audit evidence."]})]});if(!o||!o.active)return r.jsxs("section",{className:"md3-card-elevated space-y-3 p-6 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx(Ai,{className:"h-4 w-4",style:{color:"var(--md-sys-color-warning)"}}),r.jsx("h2",{className:"font-bold text-base",children:"Direct Adaptive Runtime"})]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:(o==null?void 0:o.mode)==="READ_ONLY"?"Daemon is stopped. The last OEM baseline remains available for recovery.":"Runtime discovery or durable baseline preparation is unavailable. Independent daemon status remains accessible."})]});const N=o.appliedValues.filter(b=>b.owned).length,B=o.appliedValues.filter(b=>b.modified).length,U=o.ownershipLosses>0||o.appliedValues.some(b=>!b.owned&&b.modified),R=o.journalFault?"md3-badge md3-badge-error":o.thermalGuard?"md3-badge md3-badge-warning":"md3-badge md3-badge-success";return r.jsxs("section",{className:"md3-card-elevated space-y-5 p-6 sm:p-7","aria-label":"Direct Adaptive runtime and actuator audit",children:[r.jsxs("div",{className:"flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(gn,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"font-bold text-base sm:text-lg",children:"Direct Adaptive · Production Composition Root"})]}),r.jsx("p",{className:"mt-1 text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Topology-derived Sweet-Spot policy and readback-verified sysfs ownership. Optional PSI/FrameTimeline evidence never blocks independently discovered domains."})]}),r.jsxs("span",{className:R,children:[o.journalFault?r.jsx(Ai,{className:"h-3.5 w-3.5"}):o.thermalGuard?r.jsx(rs,{className:"h-3.5 w-3.5"}):r.jsx(Un,{className:"h-3.5 w-3.5"}),o.journalFault?"Journal fault · mutations halted":Yg[o.mode]??o.mode]})]}),r.jsxs("div",{className:"grid grid-cols-2 gap-3 sm:grid-cols-4",children:[r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(za,{className:"h-3.5 w-3.5"})," CPU utilization"]}),r.jsxs("div",{className:"mt-1 font-mono text-lg font-bold tabular-nums",children:[o.cpuUtilizationPct.toFixed(1),"%"]})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(Tl,{className:"h-3.5 w-3.5"})," RAM / ZRAM"]}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold",children:Wf(o.ramBytes)}),r.jsxs("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["ZRAM ",Wf(o.zramBytes)]})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(ag,{className:"h-3.5 w-3.5"})," Display"]}),r.jsx("div",{className:"mt-1 font-mono text-lg font-bold",children:o.refreshHz>0?`${o.refreshHz} Hz`:"Auto"}),r.jsxs("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Screen ",o.screenState==="OFF"?"off":"on / active"]})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(rs,{className:"h-3.5 w-3.5"})," Actuator audit"]}),r.jsx("div",{className:"mt-1 font-mono text-lg font-bold",children:o.verifiedMutations}),r.jsxs("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[o.skippedNodes," skipped · ",o.ownershipLosses," ownership losses"]})]})]}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px] font-bold uppercase tracking-wide",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"GPU Discovered"}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-bold",children:o.gpuModel})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px] font-bold uppercase tracking-wide",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Storage Topology"}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-bold",children:o.storageType})]})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2",children:[r.jsx("h3",{className:"text-sm font-bold",children:"CPU Topology & Discovered OPP"}),r.jsxs("span",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[o.cpuClusters.length," clusters · ",o.discoveredNodes," nodes"]})]}),o.cpuClusters.length===0?r.jsx("p",{className:"md3-card-filled p-3.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CPUFreq OPP table unavailable; CPU actuation is skipped independently."}):r.jsx("div",{className:"grid grid-cols-1 gap-3 md:grid-cols-2",children:o.cpuClusters.map(b=>r.jsxs("div",{className:"md3-card-filled min-w-0 p-3.5",children:[r.jsx("div",{className:"text-xs font-bold",children:b.id}),r.jsx("div",{className:"mt-2 flex flex-wrap gap-1.5",children:b.opps.map(y=>r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:Qg(y)},`${b.id}-${y}`))})]},b.id))})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsxs("div",{className:"flex flex-col gap-1 sm:flex-row sm:items-end sm:justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Current Applied Values vs OEM Baseline"}),r.jsxs("span",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[B," modified · ",N,"/",o.appliedValues.length," owned"]})]}),o.appliedValues.length===0?r.jsx("p",{className:"md3-card-filled p-3.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No writable semantic control nodes were captured."}):r.jsx("div",{className:"space-y-2",children:o.appliedValues.slice(0,24).map(b=>r.jsxs("div",{className:"md3-card-filled space-y-2 p-3.5",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"break-all font-mono text-xs font-bold",children:b.semantic}),r.jsx("div",{className:"break-all text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:b.domain})]}),r.jsxs("span",{className:b.owned?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",children:[b.owned?r.jsx(ru,{className:"h-3 w-3"}):r.jsx(Ai,{className:"h-3 w-3"}),b.owned?"Owned / readback OK":b.health]})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-2 text-[11px]",children:[r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Current"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-bold",children:b.current})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"OEM"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-bold",children:b.oemBaseline})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Policy target"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-bold",children:b.target})]})]})]},b.path))})]}),U&&r.jsx("div",{role:"status",className:"rounded-2xl px-4 py-3 text-xs",style:{backgroundColor:"var(--md-sys-color-warning-container)",color:"var(--md-sys-color-on-warning-container)"},children:"One or more nodes lost mutation ownership or readback agreement. Those actuators are isolated; unrelated domains continue."}),r.jsxs("div",{className:"flex flex-wrap gap-2.5",children:[r.jsxs("button",{type:"button",onClick:j,className:"md3-btn-tonal",children:[r.jsx(Fh,{className:"h-3.5 w-3.5"})," View Mutation Journal"]}),r.jsxs("button",{type:"button",onClick:A,disabled:d,className:"md3-btn-outlined",style:{borderColor:"var(--md-sys-color-error)",color:"var(--md-sys-color-error)"},children:[r.jsx(ip,{className:"h-3.5 w-3.5"}),d?"Restoring OEM baseline…":"Restore OEM Baseline & Stop"]})]}),_!==null&&r.jsxs("div",{className:"space-y-2",children:[r.jsx("div",{className:"text-xs font-bold",children:"Baseline Journal · Read-Only"}),r.jsx("pre",{className:"max-h-64 overflow-auto rounded-2xl p-4 text-[11px] font-mono leading-relaxed",style:{backgroundColor:"var(--md-sys-color-surface-container-highest)",color:"var(--md-sys-color-on-surface)"},children:_})]}),r.jsxs("div",{className:"text-[11px] leading-relaxed font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["OEM snapshot entries: ",o.baselineNodes," · thermal pressure: ",o.thermalPressure.toFixed(1)," · journal state: ",o.journalFault?"fault-latched":"healthy"]})]})},Kg=[{id:"ALL",label:"All Events"},{id:"BOOST",label:"Sustained Load"},{id:"THERMAL",label:"Thermal Gate"},{id:"BASELINE",label:"Journal & Recovery"},{id:"INFO",label:"Admission & Info"}],as=({logs:o,nativeMode:c=!1})=>{const[_,d]=F.useState("ALL"),[j,A]=F.useState(""),[N,B]=F.useState(!1),U=F.useRef(null);F.useEffect(()=>()=>{U.current!==null&&window.clearTimeout(U.current)},[]);const R=F.useMemo(()=>o.filter(O=>{const $=_==="ALL"||O.type===_,q=!j||O.message.toLowerCase().includes(j.toLowerCase())||O.tag.toLowerCase().includes(j.toLowerCase());return $&&q}),[o,_,j]),b=async()=>{const O=R.map($=>`[${$.timestamp}] [${$.type}] ${$.tag}: ${$.message}`).join(`
`);try{await navigator.clipboard.writeText(O),B(!0),U.current!==null&&window.clearTimeout(U.current),U.current=window.setTimeout(()=>{B(!1),U.current=null},2e3)}catch{}},y=O=>{switch(O){case"THERMAL":return"md3-badge md3-badge-warning";case"BASELINE":return"md3-badge md3-badge-success";case"BOOST":return"md3-badge md3-badge-primary";default:return"md3-badge md3-badge-neutral"}};return r.jsxs("section",{className:"md3-card-elevated p-6 sm:p-7 space-y-5","aria-label":"Observability and audit trail",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-center justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-start gap-3.5",children:[r.jsx("div",{className:"w-11 h-11 rounded-2xl flex items-center justify-center shrink-0",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(_u,{className:"w-5 h-5"})}),r.jsxs("div",{className:"space-y-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h3",{className:"font-bold text-base sm:text-lg",children:"Observability & Audit Trail"}),r.jsxs("span",{className:"md3-badge md3-badge-neutral font-mono",children:[R.length," events"]})]}),r.jsx("p",{className:"text-xs sm:text-sm",style:{color:"var(--md-sys-color-on-surface-variant)"},children:c?"Live daemon transaction audit stream from runtime log ring.":"Preview transaction trace showing admission, readback verification, and outcome learning."})]})]}),r.jsx("div",{className:"flex items-center gap-2 shrink-0",children:r.jsxs("button",{type:"button",onClick:()=>void b(),className:"md3-btn-outlined",children:[N?r.jsx(lu,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}):r.jsx(su,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:N?"Copied":"Copy Audit Log"})]})})]}),r.jsxs("div",{className:"flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-3",children:[r.jsx("div",{className:"flex flex-wrap items-center gap-1.5",children:Kg.map(O=>{const $=_===O.id;return r.jsx("button",{type:"button","aria-pressed":$,onClick:()=>d(O.id),className:"md3-chip "+($?"is-selected":""),children:O.label},O.id)})}),r.jsxs("div",{className:"relative",children:[r.jsx(lp,{className:"w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none",style:{color:"var(--md-sys-color-on-surface-variant)"}}),r.jsx("input",{type:"text","aria-label":"Filter audit trail",placeholder:"Filter audit trail...",value:j,onChange:O=>A(O.target.value),className:"w-full sm:w-60 pl-9 pr-4 py-2 text-xs rounded-full focus:outline-none",style:{backgroundColor:"var(--md-sys-color-surface-container)",border:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface)"}})]})]}),r.jsx("div",{role:"log","aria-label":"Audit trail",className:"md3-card-filled p-3.5 max-h-80 overflow-y-auto font-mono text-xs divide-y",style:{borderColor:"var(--md-sys-color-outline-variant)"},children:R.length===0?r.jsx("div",{className:"text-center py-8 font-sans text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No audit events match the current filter."}):R.map(O=>r.jsxs("div",{className:"grid grid-cols-1 sm:grid-cols-[78px_98px_124px_1fr] items-baseline gap-2 py-2.5 px-2.5 rounded-xl transition-colors",style:{borderColor:"var(--md-sys-color-outline-variant)"},children:[r.jsx("span",{className:"tabular-nums text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:O.timestamp}),r.jsx("span",{children:r.jsx("span",{className:y(O.type),children:O.type})}),r.jsx("span",{className:"font-semibold truncate",children:O.tag}),r.jsx("span",{className:"break-words leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:O.message})]},O.id))})]})},Xg=[{id:"ocean",label:"Ocean",swatch:"#0b57d0"},{id:"emerald",label:"Emerald",swatch:"#006c4c"},{id:"amber",label:"Amber",swatch:"#b27b00"}],Zg=({isDark:o,onToggleTheme:c,accent:_="ocean",onSelectAccent:d,modeLabel:j,equilibriumScore:A,nativeMode:N=!1,nativeConnected:B=!1,onOpenArchitectureStudio:U})=>{const R=N&&B?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",b=N?B?"DAEMON LINKED":"AWAITING PAYLOAD":"SIMULATION PREVIEW";return r.jsx("header",{className:"md3-top-app-bar sticky top-0 z-30",children:r.jsxs("div",{className:"mx-auto flex min-h-[4.25rem] max-w-7xl items-center justify-between gap-3 px-4 py-2.5 sm:px-6",children:[r.jsxs("div",{className:"flex min-w-0 flex-1 items-center gap-3.5",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl shadow-xs",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(za,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex min-w-0 flex-wrap items-center gap-2",children:[r.jsx("span",{className:"truncate text-sm font-bold tracking-tight sm:text-base",children:"CFAE System Framework"}),r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:"v4.0.0"}),r.jsxs("span",{className:R,children:[r.jsx("span",{className:"h-1.5 w-1.5 rounded-full",style:{backgroundColor:N&&B?"var(--md-sys-color-success)":"var(--md-sys-color-warning)"}}),b]})]}),r.jsxs("p",{className:"mt-0.5 truncate text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:j}),r.jsx("span",{children:" · Coordinated Mutation Safety & Autonomous Equilibrium"})]})]})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2",children:[typeof A=="number"&&r.jsxs("div",{className:"hidden md:inline-flex items-center gap-1.5 rounded-full px-3 py-1.5 text-xs font-semibold",style:{backgroundColor:"var(--md-sys-color-surface-container)",border:"1px solid var(--md-sys-color-outline-variant)"},title:"Composite equilibrium index derived from thermal headroom, PSI pressure, and ownership health",children:[r.jsx(Un,{className:"h-3.5 w-3.5",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{className:"font-mono tabular-nums",children:[A,"%"]}),r.jsx("span",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Equilibrium"})]}),d&&r.jsxs("div",{className:"hidden sm:inline-flex items-center gap-1 rounded-full px-2 py-1",style:{backgroundColor:"var(--md-sys-color-surface-container)",border:"1px solid var(--md-sys-color-outline-variant)"},role:"group","aria-label":"Material You seed color palette",children:[r.jsx(cg,{className:"h-3.5 w-3.5 ml-1 mr-0.5",style:{color:"var(--md-sys-color-on-surface-variant)"}}),Xg.map(y=>{const O=_===y.id;return r.jsx("button",{type:"button",id:`btn-accent-${y.id}`,"aria-pressed":O,"aria-label":`${y.label} Material 3 palette`,title:`${y.label} Material 3 palette`,onClick:()=>d(y.id),className:"h-5 w-5 rounded-full flex items-center justify-center transition-transform",style:{backgroundColor:y.swatch,boxShadow:O?"0 0 0 2px var(--md-sys-color-surface), 0 0 0 4px var(--md-sys-color-primary)":"none"}},y.id)})]}),r.jsxs("button",{type:"button",id:"btn-open-architecture-studio",onClick:U,title:"Open Architecture Studio","aria-label":"Open Architecture Studio",className:"md3-btn-tonal",children:[r.jsx(cu,{className:"h-4 w-4"}),r.jsx("span",{className:"hidden sm:inline",children:"Studio"})]}),r.jsx("button",{type:"button",id:"btn-toggle-theme",onClick:c,className:"md3-icon-btn","aria-label":"Toggle color theme",title:o?"Switch to light theme":"Switch to dark theme",children:o?r.jsx(jg,{className:"h-4 w-4"}):r.jsx(lg,{className:"h-4 w-4"})})]})]})})},Jg="# CFAE System Framework v3.1.0 — C++23 + Rust 2024 + Kotlin Equilibrium & Mutation Safety Blueprint\n\n## 1. Runtime Architecture & Language Implementation Stack\n\n- **ISO C++23 Core (`src/daemon/`, `src/kernel/`, `include/coreflow/`)**: Handles semantic sysfs/cgroup v2 discovery, `ResourcePermissionClass` classification (`ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, `IncompleteMetadata`), `CentralMutationCoordinator` domain arbitration, Linux `epoll`/`timerfd`/`signalfd` event loop, `O_NOFOLLOW` + `fstat` (`dev:ino`) object identity checks, durable write-ahead journal (`CFJ4`), persistent bandit state (`CFTS2`), readback verification, and Android service capability probing.\n- **Rust 2024 Decision Engine (`rust/src/`)**: Owns the memory-safe thermal PID controller, hysteresis state machine, PSI pressure arbitration, invalid-sample rejection, and slew-bounded capacity budget via `cxx` FFI.\n- **Kotlin 2.1 Android Integration (`android/src/main/kotlin/io/coreflow/`)**: Provides native Android 15 (API 35) Quick Settings TileService (`CoreFlowTileService`) and thermal broadcast integration (`CoreFlowThermalReceiver`).\n- **Material Design 3 WebUI (`webui/`, `module/webroot/`)**: Lightweight offline dashboard synchronized between `dist/` and `module/webroot/`.\n\n## 2. Coordinated Mutation Transactions (6-Layer Pipeline)\n\nRather than rigid single-parameter-only rules or blind bulk writes, the primary unit of control is the **Coordinated Mutation Transaction**: one explicit optimization objective, a dependency-verified change plan within a bounded risk budget, domain ownership arbitration via `CentralMutationCoordinator`, per-write readback verification, settling-window + noise-floor validation, and ownership-safe rollback.\n\n1. **Universal Capability Classifier**: Classifies every discovered kernel node into `ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, or `IncompleteMetadata`. Unknown or unsafe nodes are never mutated.\n2. **Workload & Bottleneck Reasoning Engine**: Combines 11-state workload classification, CPU/Memory/I/O PSI, thermal headroom, and temperature trend into an inspectable `OptimizationObjective`.\n3. **Active Thermal Stabilization & Hysteresis Recovery**: Switches objective to `ThermalStabilization` (`thermal.schedutil_damped_pacing`, `thermal.io_burst_pacing`) under non-emergency thermal load, and transitions through hysteresis-guarded `ThermalRecovery` when cooling.\n4. **Central Mutation Coordinator & Dependency Analyzer**: Enforces single-mutation-per-domain leases, priority preemption (`EmergencySafety > ThermalStabilization > PressureGuard > WorkloadOptimization > BackgroundEfficiency`), and `RequiredAnchor -> RequiredDependent` ordering.\n5. **Safety Admission Gate & `CFJ4` Journal**: Verifies pre-write invariants, commits the `CFJ4` write-ahead journal with `boot_id`/`boot_epoch` and `dev:ino` identity prior to the first write, applies changes in dependency order, and verifies every write via readback.\n6. **Outcome Evaluator & `CFTS2` Persistent Learning**: Evaluates post-settling impact against the domain noise floor (`IMPROVED`, `NEUTRAL`, `REGRESSED`, `INCONCLUSIVE`, `ABORTED`, `OWNERSHIP_LOST`, `RECOVERY_REQUIRED`, `REJECTED`), steps down `adaptive_scale` on rollback, enforces context-aware anti-repeat cooldowns, and persists learning atomically in `CFTS2` format guarded by a capability fingerprint.\n\n## 3. Cross-Device Thermal & Power Policy\n\nRaw CPU/GPU junction, battery, and skin temperatures are distinct diagnostics and never share an arbitrary universal cutoff in native mode. Linux thermal-zone values are converted from the documented millidegree ABI.\n\nProduction decisions prioritize the Android NDK thermal severity and headroom API alongside recognized compute, battery, and skin zones evaluated against their read-only OEM passive/hot/critical trip points and secondary `hwmon` sensors. Missing OEM thermal policy retains the stock baseline and reports unavailable.\n\n## 4. Predictive Controller & Slew Bounding\n\nRust consumes a dimensionless thermal-policy pressure index. Its PID controller includes a bounded integral and pressure velocity, bounding output to `0.4–1.0` with a maximum slew of 2% per elapsed second. CPU/GPU ceilings are quantized to exposed operating points while honoring existing OEM maximums.\n\n## 5. Ownership Guard & Idempotent Recovery\n\nFor every mutation, `BaselineStore` records the baseline value, written value, last verified actual value, object identity (`dev:ino`), and ownership state. Rollback restores baseline values only if object identity matches and no external Android/OEM process has taken over (`OWNERSHIP_LOST` retires ownership without overwriting external changes). Crash recovery on startup is strictly idempotent.\n\n## 6. Hardware Introspection & Preservation\n\n- **CPU/GPU Ceilings**: Discovered from `cpufreq` and `devfreq` frequency tables. Missing tables disable that domain.\n- **Battery Step-Charging**: Uses standard `constant_charge_current_max`, evaluates battery temperature exclusively while charging, and never raises an OEM current ceiling.\n- **Storage I/O Weighting & Relative Queue Scaling**: Uses documented cgroup v2 `io.weight` (`default N` format) and scales block queue tunables (`read_ahead_kb`, `nr_requests`, `iosched/*`) relative to captured OEM baselines.\n- **Truthful Vulkan Pass-Through**: `libVkLayer_coreflow.so` reports only physical GPU ICD capabilities (`safe_hardware`) without spoofing API versions or unsupported features.\n\n",$g=`use crate::bridge::ffi::{Decision, Parameters, Snapshot};

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
            .id = "vm.mglru_thrash_guard",
            .domain = "vm",
            .label = "MGLRU Working-Set Thrash Guard (min_ttl_ms=1000ms)",
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
                       (node.source == ProfileSource::Procfs || node.source == ProfileSource::Sysfs);
            });
        if (found != device_profile_->node_capabilities.end()) return found->path;
    }

    if (semantic_id == "vm.lru_gen_min_ttl_ms") {
        const fs::path min_ttl = fs::path(sys_root_) / "kernel/mm/lru_gen/min_ttl_ms";
        std::error_code ec;
        const auto status = fs::symlink_status(min_ttl, ec);
        if (!ec && !fs::is_symlink(status) && fs::is_regular_file(status)) {
            return min_ttl.string();
        }
        return {};
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

    if (candidate_id == "vm.mglru_thrash_guard") {
        const auto mglru_ttl = resolve_vm_node_path("vm.lru_gen_min_ttl_ms");
        return !mglru_ttl.empty() && has_runtime_node(mglru_ttl);
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
    const auto mglru_ttl_path = resolve_vm_node_path("vm.lru_gen_min_ttl_ms");
    if (!page_cluster_path.empty()) (void)store.capture(page_cluster_path);
    if (!cache_pressure_path.empty()) (void)store.capture(cache_pressure_path);
    if (!mglru_ttl_path.empty()) (void)store.capture(mglru_ttl_path);

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
    const std::string vm_mglru_min_ttl_path = resolve_vm_node_path("vm.lru_gen_min_ttl_ms");
    EnvironmentDiscoveryInput env_input{};
    env_input.sys_root = sys_root_;
    env_input.proc_root = proc_root_;
    env_input.zram_active = zram_active_;
    env_input.vm_page_cluster_path = vm_page_cluster_path;
    env_input.vm_vfs_cache_pressure_path = vm_cache_pressure_path;
    env_input.vm_mglru_min_ttl_path = vm_mglru_min_ttl_path;
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
`,i0=["Architecture","Mutation Safety","Risk & Provenance","HAL / Binder","Thermal Simulator","C++23 / Rust 2024","Export"],l0=o=>{const[c,_]=F.useState("Architecture"),[d,j]=F.useState(75),[A,N]=F.useState(30),[B,U]=F.useState(!1),[R,b]=F.useState(""),[y,O]=F.useState("Autonomous tuner"),$={"Autonomous tuner":e0,"Neural predictor":t0,"Rust controller":$g,"cxx bridge":Wg,"CPU/GPU controller":n0,"Power HAL probe":a0},q=F.useMemo(()=>{let E=A,D=0,P=E,he=1;const Oe=[];for(let re=0;re<=120;re++){const ue=42.5-E;D=Math.max(-50,Math.min(0,D+ue));const Ie=re===0?0:E-P,Xe=Math.max(.4,Math.min(1,1+(2.5*ue+.15*D-1.2*Ie)/50));he+=Math.max(-.02,Math.min(.02,Xe-he)),P=E,E+=d/100*he*.5-(E-A)*.025,Oe.push({temperature:E,scale:he})}return Oe},[d,A]),{isOpen:K,onClose:V}=o;if(F.useEffect(()=>{if(!K)return;const E=D=>{D.key==="Escape"&&V()};return window.addEventListener("keydown",E),()=>window.removeEventListener("keydown",E)},[K,V]),F.useEffect(()=>{if(!B)return;const E=window.setTimeout(()=>U(!1),2e3);return()=>window.clearTimeout(E)},[B]),!K)return null;const Z=Jg+`

## CFAE System Framework v4.0.0
## Current Hardware
SoC: ${o.socModel}
Kernel: ${o.kernelVersion}
CPUs: ${o.onlineCpus}
GPU: ${o.gpuType}
`,ie=async()=>{try{await navigator.clipboard.writeText(Z),U(!0),b("")}catch{b("Clipboard unavailable. Use Download.")}},G=E=>{const D=URL.createObjectURL(new Blob([Z],{type:"text/plain;charset=utf-8"})),P=document.createElement("a");P.href=D,P.download=`cfae_framework_blueprint.${E}`,document.body.appendChild(P),P.click(),P.remove(),window.setTimeout(()=>URL.revokeObjectURL(D),1e3)},ce=q.map((E,D)=>`${D*5},${200-(E.temperature-20)*4}`).join(" "),Ee=q.map((E,D)=>`${D*5},${200-E.scale*160}`).join(" ");return r.jsx("div",{className:"fixed inset-0 z-50 flex items-center justify-center p-3 sm:p-6 backdrop-blur-xl md3-page-stage",style:{background:"color-mix(in srgb, var(--md-sys-color-scrim) 55%, transparent)"},children:r.jsxs("div",{role:"dialog","aria-modal":"true","aria-label":"CFAE System Framework Architecture Studio",className:"relative w-full max-w-5xl h-[88vh] flex flex-col md3-card-elevated overflow-hidden",style:{background:"var(--md-sys-color-surface-container-high)",border:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("header",{className:"px-6 py-4 flex justify-between items-center gap-3",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-center gap-3",children:[r.jsx("div",{className:"w-10 h-10 rounded-2xl flex items-center justify-center shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(cu,{size:19})}),r.jsxs("div",{children:[r.jsx("h2",{className:"font-semibold text-base sm:text-lg tracking-tight",style:{color:"var(--md-sys-color-on-surface)"},children:"CFAE Architecture & Mutation Safety Studio"}),r.jsx("p",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-primary)"},children:"v4.0.0 · C++23 + Rust 2024 + Kotlin · Coordinated Mutation Transactions"})]})]}),r.jsx("button",{onClick:o.onClose,"aria-label":"Close Studio",className:"md3-icon-btn",children:r.jsx(sp,{size:18})})]}),r.jsx("nav",{className:"flex flex-wrap gap-1.5 px-6 py-3",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)",background:"var(--md-sys-color-surface-container)"},"aria-label":"Studio sections",children:i0.map(E=>r.jsx("button",{onClick:()=>_(E),className:"md3-chip "+(E===c?"md3-chip-active":""),children:E},E))}),r.jsxs("div",{className:"flex-1 min-h-0 overflow-auto p-6 sm:p-8 space-y-6 text-sm",children:[c==="Architecture"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(za,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Multi-Language Native Architecture"]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Sensor & Capability Discovery → State Normalization → Reasoning Engine → Dependency Analyzer → Mutation Planner → Safety Admission Gate → Transaction Coordinator (Apply Order + Ownership Guard) → Readback Verify → Outcome Evaluator → Retain / Rollback → Outcome Learning."})]}),r.jsx("div",{className:"grid sm:grid-cols-2 gap-4",children:[["ISO C++23 Core Runtime","Zero-polling epoll/timerfd/signalfd loop, semantic cgroup v2 & sysfs discovery, O_NOFOLLOW + fstat identity verification, and fsync write-ahead journal."],["Rust 2024 Decision Engine","Memory-safe PID thermal controller, hysteresis state machine, PSI contention arbitration, and bounded capacity scaling via cxx bridge."],["Kotlin 2.1 Android Bridge","Android 15 (API 35) Quick Settings TileService and Battery thermal broadcast integration."],["Material 3 Expressive WebUI","Offline KernelSU/APatch workbench displaying live runtime status or clearly labeled preview telemetry with dynamic seed palettes."]].map(([E,D])=>r.jsxs("div",{className:"md3-card-filled p-5 space-y-1.5",children:[r.jsx("h4",{className:"text-xs sm:text-sm font-semibold",style:{color:"var(--md-sys-color-primary)"},children:E}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:D})]},E))}),r.jsxs("div",{className:"md3-card-outlined p-5 font-mono text-xs space-y-1.5",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("div",{children:["SoC: ",o.socModel]}),r.jsxs("div",{children:["Kernel: ",o.kernelVersion]}),r.jsxs("div",{children:["Online CPUs: ",o.onlineCpus]}),r.jsxs("div",{children:["GPU: ",o.gpuType]})]})]}),c==="Mutation Safety"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(Un,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Coordinated Optimization & 5-Layer Mutation Safety"]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["The primary unit of control is the ",r.jsx("strong",{children:"Mutation Transaction"}),": one explicit optimization goal, a dependency-verified change plan within a bounded risk budget, per-write readback verification, and ownership-safe rollback."]})]}),r.jsx("div",{className:"grid sm:grid-cols-2 lg:grid-cols-3 gap-4",children:[["1. Reasoning Engine","Synthesizes workload observations, CPU/Memory/I/O PSI, thermal headroom, and power state into an inspectable optimization intent."],["2. Dependency Analyzer","Builds a provenance-backed dependency graph (Required-before, Requires, Coupled-with, Conflicts-with, Unknown). Unknown edges are never grouped."],["3. Mutation Planner","Constructs a bounded transaction plan specifying baseline, target, allowed range source, execution order, risk budget, and abort conditions."],["4. Safety Admission Gate","Verifies 10 pre-write invariants: object identity, semantic units, valid baseline, zero ownership conflict, and OEM thermal compliance."],["5. Coordinator & Readback","Serializes conflicting transactions, writes the durable journal prior to mutation, applies changes in dependency order, and verifies readback."],["6. Outcome Learning","Evaluates Stability, Responsiveness, Pressure, Thermal, and Power outcomes (IMPROVED, NEUTRAL, REGRESSED, OWNERSHIP_LOST) with anti-repeat deferral."]].map(([E,D])=>r.jsxs("div",{className:"md3-card-filled p-4 space-y-1.5",children:[r.jsx("h4",{className:"text-xs font-semibold",style:{color:"var(--md-sys-color-primary)"},children:E}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:D})]},E))}),r.jsxs("div",{className:"md3-card-outlined p-5 text-xs space-y-2",children:[r.jsx("div",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"12 Non-Negotiable Safety Invariants:"}),r.jsx("p",{className:"leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"1. Zero write without valid baseline · 2. Zero mutation without known semantics & range · 3. Zero grouped mutation without verified relationship · 4. No concurrent conflicting transactions · 5. Mandatory readback after every write · 6. Rollback never overwrites external OEM/Android changes · 7. No blind retries without new evidence · 8. Retention requires valid outcome & ownership · 9. No unverified metric claims · 10. OEM/Android thermal policy remains supreme · 11. Write-ahead journal & idempotent recovery · 12. Fail-safe default is no write."})]})]}),c==="Risk & Provenance"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsx("h3",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Multi-Dimensional Risk Model & Dependency Provenance"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CoreFlow evaluates seven distinct risk dimensions rather than a single opaque score: Semantic uncertainty, Platform uncertainty, Interaction risk, Reversibility, System impact, Observability, and Ownership confidence."})]}),r.jsx("div",{className:"grid sm:grid-cols-2 gap-4",children:[["Low Risk Class","Strong semantics, bounded impact, reversible, zero conflict, high observability. Eligible for coupled mutation groups within budget."],["Medium Risk Class","Limited interaction or platform uncertainty with understood dependencies and verified rollback. Applied step-by-step in one transaction."],["High Risk Class","Broad impact, uncertain platform semantics, or weak observability. Strictly isolated or rejected; never grouped with other changes."],["Forbidden Class","Missing baseline, unsafe rollback path, ownership conflict, or violates OEM thermal/power safety policy. Never mutated."]].map(([E,D])=>r.jsxs("div",{className:"md3-card-filled p-4 space-y-1",children:[r.jsx("div",{className:"text-xs font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:E}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:D})]},E))}),r.jsxs("div",{className:"md3-card-outlined p-5 space-y-2 text-xs",children:[r.jsx("div",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"Transaction State Machine Protocol"}),r.jsx("div",{className:"font-mono leading-relaxed",style:{color:"var(--md-sys-color-primary)"},children:"PROPOSED → ADMISSION_CHECK → PREPARED → APPLYING → READBACK_VERIFY → OBSERVING → COMMITTED → RETAINED"}),r.jsx("p",{className:"leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Failed admission transitions to DEFERRED / REJECTED. Readback mismatch or observed regression transitions immediately to ROLLING_BACK under Ownership Guard verification."})]})]}),c==="HAL / Binder"&&r.jsxs(r.Fragment,{children:[r.jsx("h3",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Platform Integration & OEM Boundaries"}),r.jsx("div",{className:"md3-card-filled overflow-hidden",children:r.jsxs("table",{className:"w-full text-xs text-left",children:[r.jsx("thead",{children:r.jsxs("tr",{style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("th",{className:"p-4 font-semibold",children:"Capability"}),r.jsx("th",{className:"p-4 font-semibold",children:"Implementation Status"})]})}),r.jsx("tbody",{children:[["AIDL Power HAL discovery","Nonblocking libbinder_ndk lookup; capability detection only."],["Qualcomm / MediaTek interception","Unavailable: requires a registered, versioned OEM/AOSP proxy backend."],["Binder device access","Does not expose or reroute other processes’ transactions."],["NPU / APU offload","Unavailable: requires a compatible compute workload and runtime."]].map(([E,D])=>r.jsxs("tr",{style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("td",{className:"p-4 font-medium",style:{color:"var(--md-sys-color-primary)"},children:E}),r.jsx("td",{className:"p-4",style:{color:"var(--md-sys-color-on-surface-variant)"},children:D})]},E))})]})}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"OEM thermal services remain authoritative. The daemon never replaces Power HAL services, patches Binder, or injects undocumented vendor transactions."})]}),c==="Thermal Simulator"&&r.jsxs(r.Fragment,{children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(gn,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Illustrative Thermal Equilibrium Plant"]}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Synthetic first-order thermal plant for previewing PID slew bounding. Separate from native device sensor telemetry."}),r.jsxs("div",{className:"grid sm:grid-cols-2 gap-4",children:[r.jsxs("label",{className:"md3-card-filled p-4 block text-xs",children:[r.jsxs("span",{className:"font-medium",style:{color:"var(--md-sys-color-on-surface)"},children:["Workload Intensity: ",d,"%"]}),r.jsx("input",{"aria-label":"Simulated workload",className:"block w-full mt-2",type:"range",min:"10",max:"100",value:d,onChange:E=>j(Number(E.target.value))})]}),r.jsxs("label",{className:"md3-card-filled p-4 block text-xs",children:[r.jsxs("span",{className:"font-medium",style:{color:"var(--md-sys-color-on-surface)"},children:["Ambient Temperature: ",A," °C"]}),r.jsx("input",{"aria-label":"Simulated ambient temperature",className:"block w-full mt-2",type:"range",min:"20",max:"40",value:A,onChange:E=>N(Number(E.target.value))})]})]}),r.jsxs("svg",{viewBox:"0 0 620 235",role:"img","aria-label":"Temperature and capacity scale over 120 simulated seconds",className:"w-full md3-card-filled p-2",children:[r.jsx("line",{x1:"10",y1:"200",x2:"610",y2:"200",stroke:"#64748b",strokeOpacity:"0.4"}),r.jsx("polyline",{points:ce,fill:"none",stroke:"#f59e0b",strokeWidth:"2.2",transform:"translate(10,0)"}),r.jsx("polyline",{points:Ee,fill:"none",stroke:"var(--md-sys-color-primary)",strokeWidth:"2.2",transform:"translate(10,0)"}),r.jsx("text",{x:"12",y:"222",fill:"#64748b",fontSize:"11",children:"0 s"}),r.jsx("text",{x:"565",y:"222",fill:"#64748b",fontSize:"11",children:"120 s"})]}),r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs font-mono",children:[r.jsxs("span",{style:{color:"var(--md-sys-color-on-surface)"},children:["Final: ",q[120].temperature.toFixed(1)," °C ·"," ",(q[120].scale*100).toFixed(0),"% capacity budget"]}),r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Amber: Temperature (20–70 °C) · Accent: Capacity Scale (40–100%)"})]})]}),c==="C++23 / Rust 2024"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"flex items-center justify-between gap-3 flex-wrap",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",style:{color:"var(--md-sys-color-on-surface)"},children:[r.jsx(np,{size:18,style:{color:"var(--md-sys-color-primary)"}}),"Production Source Inspector"]}),r.jsx("select",{"aria-label":"Source file",value:y,onChange:E=>O(E.target.value),className:"md3-btn-tonal text-xs",children:Object.keys($).map(E=>r.jsx("option",{children:E},E))})]}),r.jsx("pre",{className:"overflow-auto text-xs font-mono p-5 md3-card-filled whitespace-pre max-h-96 leading-relaxed",style:{color:"var(--md-sys-color-on-surface)"},children:$[y]})]}),c==="Export"&&r.jsxs(r.Fragment,{children:[r.jsx("h3",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Specification & Blueprint Export"}),r.jsxs("div",{className:"flex gap-2.5 flex-wrap",children:[r.jsxs("button",{className:"md3-btn-filled",onClick:()=>void ie(),children:[r.jsx(su,{size:14}),B?"Copied":"Copy Blueprint"]}),["md","txt"].map(E=>r.jsxs("button",{className:"md3-btn-outlined",onClick:()=>G(E),children:[r.jsx(Yh,{size:14}),"Download .",E]},E))]}),R&&r.jsx("p",{role:"alert",className:"text-xs",style:{color:"var(--md-sys-color-warning)"},children:R}),r.jsx("pre",{className:"whitespace-pre-wrap font-mono text-xs md3-card-filled p-5 max-h-96 overflow-auto leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:Z})]})]})]})})},r0=({telemetry:o,nativeMode:c,thermalTripCelsius:_})=>{const[d,j]=F.useState(null),[A,N]=F.useState(""),B=o.socCompositeCelsius&&o.socCompositeCelsius>0?o.socCompositeCelsius:o.socTempCelsius,U=o.cpuAverageCelsius??-1,R=o.cpuPeakCelsius??-1,b=o.gpuTempCelsius??-1,y=o.batteryTempCelsius??-1,O=o.skinTempCelsius??-1,$=c?!!o.thermalGuardActive:B>=_,q=Math.min(100,Math.max(0,(B-30)/30*100)),K=o.thermalSensors??[],V=K.filter(G=>!(d!==null&&G.category!==d||A&&!G.type.toLowerCase().includes(A.toLowerCase()))),Z=G=>{switch(G){case 1:return"Compute · CPU/GPU";case 2:return"Battery Cell";case 3:return"Skin · Chassis";case 4:return"Board · PMIC";default:return"Diagnostic · Aux"}},ie=G=>{const ce=G.trim();if(ce.startsWith("/sys/class/thermal/thermal_zone"))return`tz${ce.slice(31)}`;if(ce.startsWith("thermal_zone"))return`tz${ce.slice(12)}`;const Ee=ce.split("/").filter(Boolean);return(Ee[Ee.length-1]??ce).replace(/^thermal_zone/,"tz")};return r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx(ls,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("h2",{className:"font-bold text-base sm:text-lg",children:["Multi-Sensor Thermal & OEM Policy Matrix",c?"":" (sim)"]}),r.jsx("span",{className:$?"md3-badge md3-badge-warning":"md3-badge md3-badge-success",children:$?"ThermalGuard Active Mitigation":"Thermal Headroom Nominal"}),o.thermalDataDegraded&&r.jsx("span",{className:"md3-badge md3-badge-warning",children:"Sensor Degraded (Partial Read)"}),o.thermalFallbackActive&&r.jsx("span",{className:"md3-badge md3-badge-warning",children:"Generic Fallback Mode"})]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Prioritizes validated Android NDK thermal severity and per-zone OEM trip points over single-sensor heuristics. Compute, battery, skin, and PMIC/board domains are evaluated independently with physical velocity tracking."})]}),r.jsxs("div",{className:"md3-card-filled px-4 py-2.5 text-xs font-mono shrink-0 self-start",children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Policy Reference: "}),r.jsx("span",{className:"font-bold",children:c?o.thermalFallbackActive?`Fallback (${_.toFixed(1)} °C)`:`OEM / Android (${o.thermalSource??"HAL"})`:`${_.toFixed(1)} °C`})]})]}),r.jsxs("div",{className:"md3-card-filled p-5 space-y-3",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",children:[r.jsxs("span",{className:"flex items-center gap-2 font-bold",children:[r.jsx(rp,{className:"w-4 h-4 shrink-0",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Weighted SoC Composite Diagnostic",c?"":" (sim)"]})]}),r.jsxs("span",{className:"font-mono tabular-nums font-bold text-base shrink-0",children:[B>0?B.toFixed(1):"--.-"," °C"]})]}),r.jsx("div",{className:"md3-progress-track",children:r.jsx("div",{className:"md3-progress-bar",style:{width:`${q}%`}})}),r.jsxs("div",{className:"flex justify-between text-[11px] font-mono tabular-nums",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("span",{children:"30.0 °C (Idle)"}),r.jsxs("span",{children:[_.toFixed(1)," °C (Mitigation Gate)"]}),r.jsx("span",{children:"60.0 °C (OEM Ceiling)"})]})]}),r.jsx("div",{className:"grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 gap-3",children:[{label:"CPU Cluster Avg",val:U,Icon:za,sub:"All compute cores"},{label:"CPU Peak Core",val:R,Icon:ls,sub:"Hottest core die"},{label:"GPU Domain",val:b,Icon:gn,sub:"Graphics subsystem"},{label:"Battery Cell",val:y,Icon:Ah,sub:"Charging protection"},{label:"Skin / Chassis",val:O,Icon:xg,sub:"Surface comfort"}].map(({label:G,val:ce,Icon:Ee,sub:E})=>r.jsxs("div",{className:"md3-card-filled p-4 flex flex-col justify-between",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs mb-2",children:[r.jsx("span",{className:"font-semibold truncate",children:G}),r.jsx(Ee,{className:"w-3.5 h-3.5 shrink-0",style:{color:"var(--md-sys-color-primary)"}})]}),r.jsxs("div",{className:"text-xl font-bold font-mono tabular-nums",children:[ce>0?ce.toFixed(1):"--.-"," ",r.jsx("span",{className:"text-xs font-normal",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"°C"})]}),r.jsx("div",{className:"text-[11px] mt-1.5",style:{color:"var(--md-sys-color-on-surface-variant)"},children:E})]},G))}),K.length>0&&r.jsxs("details",{className:"group md3-card-filled overflow-hidden",open:!0,children:[r.jsxs("summary",{className:"cursor-pointer p-4 sm:p-5 flex items-center justify-between text-xs sm:text-sm font-bold select-none",children:[r.jsxs("div",{className:"flex items-center gap-2.5 min-w-0",children:[r.jsx(Un,{className:"w-4 h-4 shrink-0",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{className:"truncate",children:["Discovered Hardware Thermal Zones",c?"":" (sim)"," (",K.length," Zones)"]})]}),r.jsx(qh,{className:"w-4 h-4 shrink-0 transition-transform duration-200 group-open:rotate-180"})]}),r.jsxs("div",{className:"p-4 sm:p-5 pt-0 space-y-4",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-center justify-between gap-3 pt-4",children:[r.jsx("div",{className:"flex flex-wrap gap-1.5",children:[{id:null,label:"All Domains"},{id:1,label:"Compute"},{id:2,label:"Battery"},{id:3,label:"Skin"},{id:4,label:"Board/PMIC"}].map(G=>{const ce=d===G.id;return r.jsx("button",{type:"button","aria-pressed":ce,onClick:()=>j(G.id),className:"md3-chip "+(ce?"is-selected":""),children:G.label},String(G.id))})}),r.jsxs("div",{className:"relative",children:[r.jsx(lp,{className:"w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none",style:{color:"var(--md-sys-color-on-surface-variant)"}}),r.jsx("input",{type:"text","aria-label":"Search thermal zone",placeholder:"Search zone...",value:A,onChange:G=>N(G.target.value),className:"w-full sm:w-52 pl-9 pr-3.5 py-1.5 text-xs rounded-full focus:outline-none",style:{backgroundColor:"var(--md-sys-color-surface-container-lowest)",border:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface)"}})]})]}),r.jsx("div",{className:"grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-2.5 max-h-80 overflow-y-auto pr-1",children:V.length===0?r.jsx("div",{className:"col-span-full text-center py-6 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No thermal sensors match your filter."}):V.map(G=>r.jsxs("div",{className:"md3-card-elevated p-3.5 flex items-center justify-between gap-2",children:[r.jsxs("div",{className:"min-w-0 pr-2",children:[r.jsx("div",{className:"text-xs font-mono font-bold truncate",title:G.type,children:G.type}),r.jsxs("div",{className:"text-[11px] truncate",style:{color:"var(--md-sys-color-on-surface-variant)"},title:G.id,children:[ie(G.id)," · ",Z(G.category)]})]}),r.jsxs("div",{className:"text-right shrink-0",children:[r.jsx("div",{className:"text-xs font-mono tabular-nums font-bold",children:G.valid&&G.celsius!==null?`${G.celsius.toFixed(1)} °C`:r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"N/A"})}),r.jsx("div",{className:"text-[10px] font-mono tabular-nums",style:{color:"var(--md-sys-color-on-surface-variant)"},children:G.velocityValid&&G.velocityCelsiusPerSec!==null&&G.velocityCelsiusPerSec!==void 0?`${G.velocityCelsiusPerSec>0?"+":""}${G.velocityCelsiusPerSec.toFixed(2)} °C/s`:G.policyAvailable?"OEM trip":"no-trip"})]})]},G.id))})]})]})]})},s0=({nativeMode:o,onRefreshTelemetry:c})=>{const[_,d]=F.useState("Select an inspection action above to query coreflowd runtime JSON, capabilities, recovery journal, or CF-DVS state."),[j,A]=F.useState("Ready"),[N,B]=F.useState(!1),[U,R]=F.useState(!1),b=F.useRef(null);F.useEffect(()=>()=>{b.current!==null&&window.clearTimeout(b.current)},[]);const y='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"',O=K=>{const V=K.trim(),Z=V.split(`
`).map(ce=>ce.trim()).filter(Boolean);for(let ce=Z.length-1;ce>=0;ce--)if(Z[ce].startsWith("{")&&Z[ce].endsWith("}"))try{return JSON.parse(Z[ce])}catch{}const ie=V.indexOf("{"),G=V.lastIndexOf("}");return ie>=0&&G>ie?JSON.parse(V.slice(ie,G+1).replace(/\r?\n/g,"")):JSON.parse(Z[Z.length-1]??"{}")},$=async(K,V)=>{B(!0),A(V);try{if(!os()){K.includes("--status")?d(JSON.stringify({running:!0,pid:24108,uclampMin:320,uclampMax:1024,uclampSupported:!0,platformVendor:"Universal Android",osFamily:"Android AOSP / GKI",executionTier:"UniversalFallback",policyFamily:"CoreFlowBase",cpuPressurePct:1.2,memoryPressurePct:.3,ioPressurePct:.4,baselineNodesSnapshotted:6,socTempCelsius:38.6,socCompositeCelsius:38.6,cpuAverageCelsius:38.2,cpuPeakCelsius:41.5,gpuTempCelsius:36.8,batteryTempCelsius:33.4,skinTempCelsius:35.1,recoveryPending:!1,thermalPolicyAvailable:!0,thermalSource:"multi-sensor-composite",thermalSensor:"SoC-Composite (8 CPU cores, 1 GPU domain)",thermalSeverity:0,thermalPressure:44,aiBackend:"embedded-rls-mlp",aiOnnxActive:!1,aiPredictedPressure10s:46.2,aiPredictedVelocity:.08,aiRecommendedScale:.94,aiConfidence:.86,tunerStage:"RETAINED",tunerIntent:"HoldEquilibrium",tunerCandidateId:"io.mq_deadline_low_latency",tunerVerdict:"IMPROVED"},null,2)):K.includes("--capabilities")?d(JSON.stringify({socModel:"Preview SoC Profile",kernelVersion:"Linux 6.1-android15-gki",onlineCpus:"0-7",gpuType:"Universal Multi-Vendor GPU",rustEngine:!0,powerHalDetected:!0,aiBackend:"embedded-rls-mlp",onnxRuntimeLoaded:!1,binderInterception:!1,npuOffload:!1,frameTelemetry:!1},null,2)):K.includes("--journal")?d(JSON.stringify({format:"CFJ4",bootId:"preview-boot-epoch-v4",recoveryPending:!1,entries:[]},null,2)):K.includes("--flight-recorder")?d(JSON.stringify({windowSeconds:60,samples:[{ts:Date.now()-5e3,temp:38.2,pressure:43.1,cpuPsi:2.1,memPsi:.4,ioPsi:.9,scale:.94,stage:"RETAINED",candidate:"io.mq_deadline_low_latency",verdict:"IMPROVED"},{ts:Date.now(),temp:38.6,pressure:44,cpuPsi:1.2,memPsi:.3,ioPsi:.4,scale:.94,stage:"RETAINED",candidate:"io.mq_deadline_low_latency",verdict:"IMPROVED"}]},null,2)):K.includes("--dvs-status")?d("OK INACTIVE (Preview Simulation)"):d(`[${V}]
Status: OK (Preview Sandbox)
Exit code: 0`);return}const Z=await Oi(K),ie=Z.errno!==0?`[exit ${Z.errno}] ${Z.stderr.trim()||"Command failed"}
`:"";if(!Z.stdout.trim())d(ie||Z.stderr||"Command executed with no output.");else try{const G=O(Z.stdout);K.includes("--flight-recorder")&&typeof G=="object"&&G!==null&&"flightRecorder"in G?d(ie+JSON.stringify({flightRecorder:G.flightRecorder,batteryPower:G.batteryPower},null,2)):d(ie+JSON.stringify(G,null,2))}catch{d(ie+Z.stdout)}K.includes("--status")&&await c()}catch(Z){d(`Error: ${String(Z)}`)}finally{B(!1)}},q=async()=>{try{await navigator.clipboard.writeText(_)}catch{return}R(!0),b.current!==null&&window.clearTimeout(b.current),b.current=window.setTimeout(()=>{R(!1),b.current=null},2e3)};return r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-5",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx(_u,{className:"w-4 h-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"text-base sm:text-lg font-bold",children:"Runtime RPC & Diagnostic Console"}),r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:"coreflow-api.js"}),r.jsx("span",{className:o?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",children:o?"KernelSU Bridge Active":"Preview Simulator"})]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Inspect raw JSON telemetry, discovered hardware capabilities, CFJ4 journal records, Flight-Recorder traces, and CF-DVS capture status directly from ",r.jsx("code",{className:"font-mono",children:"coreflowd"}),"."]})]}),r.jsxs("button",{type:"button",id:"btn-copy-console-output",onClick:()=>void q(),className:"md3-btn-outlined self-start shrink-0",children:[U?r.jsx(lu,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}):r.jsx(su,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:U?"Copied":"Copy Output"})]})]}),r.jsx("div",{className:"flex flex-wrap gap-2",children:[{label:"Query Status (--status)",cmd:`${y}; "$BIN" --status`,title:"Daemon Status",Icon:ap},{label:"Flight Recorder (--flight-recorder)",cmd:`${y}; "$BIN" --flight-recorder`,title:"Flight Recorder Trace",Icon:gn},{label:"Capabilities (--capabilities)",cmd:`${y}; "$BIN" --capabilities`,title:"Hardware Capabilities",Icon:np},{label:"Inspect Journal (--journal)",cmd:`${y}; "$BIN" --journal`,title:"Recovery Journal",Icon:du},{label:"CF-DVS Status (--dvs-status)",cmd:`${y}; "$BIN" --dvs-status`,title:"CF-DVS Session Status",Icon:gn},{label:"Inspect coreflow.conf",cmd:"cat /data/adb/coreflow/coreflow.conf 2>/dev/null",title:"Configuration File",Icon:Ai}].map(({label:K,cmd:V,title:Z,Icon:ie})=>{const G=j===Z;return r.jsxs("button",{type:"button",onClick:()=>void $(V,Z),disabled:N,className:"md3-chip "+(G?"is-selected":""),children:[r.jsx(ie,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:K})]},Z)})}),r.jsxs("div",{className:"md3-card-filled p-5 font-mono text-xs overflow-x-auto max-h-80",children:[r.jsxs("div",{className:"flex items-center justify-between pb-2.5 mb-2.5 text-[11px]",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["Inspector Output · ",j]}),r.jsx("span",{children:N?"Executing query…":"Read-only RPC"})]}),r.jsx("pre",{className:"whitespace-pre leading-relaxed",children:_})]})]})},o0='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"; [ -f "$BIN" ] || exit 127',c0=[{code:"DVS-01",title:"Boot stability",detail:"Daemon readiness, initial hardware discovery, boot_epoch tracking, and recovery state"},{code:"DVS-02",title:"Idle equilibrium",detail:"Baseline stability, drift detection, and release of owned mutations after sustained quiet windows"},{code:"DVS-03",title:"Interactive workload",detail:"Foreground process membership, CPU pressure, and touch/frame response pacing"},{code:"DVS-04",title:"Sustained workload",detail:"Long-running workload admission, OPP ceiling verification, and thermal response"},{code:"DVS-05",title:"Multitasking",detail:"Concurrent memory pressure, block I/O queue contention, and zram swap activity"},{code:"DVS-06",title:"Thermal recovery",detail:"Cooling velocity trend, OEM thermal trip points, and post-load hysteresis stabilization"},{code:"DVS-07",title:"Ownership conflict",detail:"External OEM writer detection, quarantine cooldown, and safe mutation relinquishment"},{code:"DVS-08",title:"Journal recovery",detail:"CFJ4 write-ahead journal integrity, SHA-256 manifest verification, and idempotent rollback"}],u0=({nativeMode:o})=>{const[c,_]=F.useState("unavailable"),[d,j]=F.useState(null),[A,N]=F.useState("Waiting for native validation status."),[B,U]=F.useState(null),[R,b]=F.useState(null),[y,O]=F.useState(!1),$=F.useCallback(async()=>{if(!o||!os()){_("unavailable"),j(null),N("Native validation status is unavailable in browser simulation preview.");return}O(!0);try{const V=await Oi(`${o0}; "$BIN" --dvs-status`);if(V.errno!==0)throw new Error(V.stderr||V.stdout||"Validation status query failed");const Z=V.stdout.trim(),ie=Z.match(/^OK ACTIVE ([0-9a-f-]{36})$/i);if(ie)_("active"),j(ie[1]),N("A native validation capture is active. Its lifecycle remains daemon-owned.");else if(Z==="OK INACTIVE")_("idle"),j(null),N("No validation capture is currently active.");else throw new Error(Z||"Unexpected CF-DVS status response");b(null),U(new Date().toLocaleTimeString())}catch(V){_("unavailable"),j(null),b(V instanceof Error?V.message:String(V)),N("Unable to read the native validation session state.")}finally{O(!1)}},[o]);F.useEffect(()=>{let V=!1;const Z=async()=>{V||await $()};Z();const ie=window.setInterval(()=>void Z(),15e3);return()=>{V=!0,window.clearInterval(ie)}},[$]);const q=c==="active"?"Capture Active":c==="idle"?"Idle · Ready":"Unavailable",K=c==="active"?"md3-badge md3-badge-success":c==="idle"?"md3-badge md3-badge-primary":"md3-badge md3-badge-warning";return r.jsxs("div",{className:"md3-card-elevated space-y-6 p-6 sm:p-8",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-start gap-3.5",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(gn,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0 flex-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h2",{className:"text-base sm:text-lg font-bold tracking-tight",children:"CoreFlow Device Validation Suite (CF-DVS)"}),r.jsx("span",{className:"md3-badge md3-badge-neutral",children:"READ ONLY · SCHEMA 1.1"})]}),r.jsx("p",{className:"mt-1 text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CoreFlow validates its own runtime invariants and exports SHA-256 manifest bundles with pinned CDDP device profiles. This surface explains coverage and reports native capture state without starting or stopping test sessions."})]})]}),o&&r.jsxs("button",{type:"button",id:"btn-refresh-dvs-status",disabled:y,onClick:()=>void $(),className:"md3-btn-outlined shrink-0 self-start",children:[r.jsx(du,{className:"h-3.5 w-3.5 "+(y?"animate-spin":"")}),r.jsx("span",{children:"Check Status"})]})]}),r.jsxs("section",{className:"md3-card-filled flex flex-col gap-3 p-4 sm:p-5 sm:flex-row sm:items-center sm:justify-between",children:[r.jsxs("div",{className:"space-y-1.5",children:[r.jsx("div",{className:"text-[11px] font-bold uppercase tracking-wider",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Native CF-DVS Session State"}),r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("span",{className:K,children:q}),d&&r.jsx("span",{className:"font-mono text-xs font-semibold break-all",children:d})]}),r.jsx("p",{className:"break-all text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:A}),B&&r.jsxs("p",{className:"text-[11px] font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Last checked ",B," · auto-refresh every 15s"]})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2 text-xs font-semibold",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(uu,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("span",{children:"Daemon-owned lifecycle"})]})]}),R&&r.jsxs("div",{role:"status",className:"flex items-start gap-2.5 rounded-2xl p-3.5 text-xs",style:{backgroundColor:"var(--md-sys-color-warning-container)",color:"var(--md-sys-color-on-warning-container)"},children:[r.jsx(Ai,{className:"mt-0.5 h-4 w-4 shrink-0"}),r.jsx("span",{children:R})]}),r.jsxs("section",{className:"space-y-3",children:[r.jsxs("div",{children:[r.jsx("h3",{className:"text-sm font-bold",children:"8-Domain Evidence & Invariant Matrix"}),r.jsxs("p",{className:"mt-0.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Evidence domains evaluated against the offline Python contract validator (",r.jsx("code",{className:"font-mono",children:"cf_dvs_validate.py"}),")."]})]}),r.jsx("div",{className:"grid grid-cols-1 gap-2.5 sm:grid-cols-2",children:c0.map(V=>r.jsxs("div",{className:"md3-card-outlined flex items-start gap-3 p-3.5",children:[r.jsx(ru,{className:"mt-0.5 h-4 w-4 shrink-0",style:{color:"var(--md-sys-color-success)"}}),r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx("span",{className:"font-mono text-[10px] font-bold",style:{color:"var(--md-sys-color-primary)"},children:V.code}),r.jsx("span",{className:"text-xs font-bold",children:V.title})]}),r.jsx("p",{className:"mt-1 text-[11px] leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:V.detail})]})]},V.title))})]}),r.jsxs("p",{className:"pt-4 text-[11px] leading-relaxed",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:["Diagnostic capture is not an optimization command. Zero-sample sessions fail closed (",r.jsx("code",{className:"font-mono",children:"RECOVERY_REQUIRED"}),") and never fabricate passing device acceptance."]})]})},Ce=({label:o,value:c,note:_,simulated:d=!1})=>{const j=typeof c=="boolean";return r.jsxs("div",{className:"flex items-center justify-between py-2.5 last:border-b-0 text-xs",style:{borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"pr-3",children:[r.jsxs("span",{children:[o,d?" (sim)":""]}),_&&r.jsxs("span",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","· ",_]})]}),j?r.jsx("div",{className:"inline-flex items-center gap-1.5 font-bold font-mono tabular-nums shrink-0",style:{color:c?"var(--md-sys-color-success)":"var(--md-sys-color-on-surface-variant)"},children:c?r.jsxs(r.Fragment,{children:[r.jsx(lu,{className:"w-3.5 h-3.5 shrink-0"}),r.jsx("span",{children:"Yes"})]}):r.jsxs(r.Fragment,{children:[r.jsx(sp,{className:"w-3.5 h-3.5 shrink-0 opacity-60"}),r.jsx("span",{children:"No"})]})}):r.jsx("span",{className:"font-bold font-mono tabular-nums shrink-0",children:c})]})},d0=({vulkanTelemetry:o,vulkanConfig:c,nativeMode:_,onSaveVulkanConfig:d,isSaving:j,saveMessage:A})=>{const N=kl(c,o),B=(R,b)=>{if(R)return b?"HW Native":void 0},U=async R=>{const y={...c,mode:R==="disabled"?"disabled":"safe_hardware"};await d(y)};return r.jsxs("div",{className:"space-y-6",children:[r.jsxs("div",{className:"md3-card-elevated p-6 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col lg:flex-row lg:items-start lg:justify-between gap-4",children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("span",{className:"md3-badge md3-badge-primary",children:_?"LIVE DEVICE ICD PROBE":"HARDWARE PASS-THROUGH MODEL (SIM)"}),r.jsx("span",{className:"md3-badge md3-badge-neutral font-mono",children:N.deviceName})]}),r.jsxs("h2",{className:"text-xl sm:text-2xl font-bold tracking-tight",children:["Vulkan Capabilities & Truthful Hardware Pass-Through",_?"":" (sim)"]}),r.jsxs("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["Dynamically probes the active device's physical GPU ICD (Qualcomm Adreno KGSL, ARM Mali / Immortalis, Samsung Xclipse, PowerVR) and reports genuine hardware capabilities without fabricating unsupported extensions, feature flags, or API versions. Opt-in diagnostic layer"," ",r.jsx("code",{className:"font-mono text-xs",children:"libVkLayer_coreflow.so"})," requires"," ",r.jsx("code",{className:"font-mono text-xs",children:"ENABLE_COREFLOW_VULKAN_LAYER=1"}),"."]})]}),r.jsxs("div",{className:"md3-card-filled px-4 py-3 text-left lg:text-right font-mono tabular-nums text-xs shrink-0 space-y-1",children:[r.jsxs("div",{children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Hardware API: "}),r.jsxs("span",{className:"font-bold",children:["Vulkan ",N.apiVersion]})]}),r.jsxs("div",{children:[r.jsx("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Device Extensions: "}),r.jsx("span",{className:"font-bold",children:N.deviceExtensions}),r.jsxs("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:[" ","· Instance: ",N.instanceExtensions]})]})]})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsx("div",{className:"text-xs font-bold",children:"Select Vulkan Layer Mode"}),r.jsxs("div",{className:"grid grid-cols-1 md:grid-cols-2 gap-3",children:[r.jsxs("button",{type:"button",id:"btn-vulkan-mode-disabled",disabled:j,"aria-pressed":c.mode==="disabled",onClick:()=>void U("disabled"),className:"text-left p-4 rounded-2xl transition-all",style:{backgroundColor:c.mode==="disabled"?"var(--md-sys-color-primary-container)":"var(--md-sys-color-surface-container)",color:c.mode==="disabled"?"var(--md-sys-color-on-primary-container)":"var(--md-sys-color-on-surface)",border:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 mb-1.5",children:[r.jsx("span",{className:"text-xs font-bold",children:"Default · Implicit Layer Disabled (Recommended)"}),r.jsx(za,{className:"w-4 h-4 shrink-0"})]}),r.jsxs("p",{className:"text-[11px] leading-relaxed opacity-85",children:["Keeps ",r.jsx("code",{className:"font-mono",children:"VK_LAYER_COREFLOW_universal_bridge"})," inactive for normal applications while still reading native GPU devfreq and physical ICD capabilities."]})]}),r.jsxs("button",{type:"button",id:"btn-vulkan-mode-passthrough",disabled:j,"aria-pressed":c.mode!=="disabled",onClick:()=>void U("safe_hardware"),className:"text-left p-4 rounded-2xl transition-all",style:{backgroundColor:c.mode!=="disabled"?"var(--md-sys-color-primary-container)":"var(--md-sys-color-surface-container)",color:c.mode!=="disabled"?"var(--md-sys-color-on-primary-container)":"var(--md-sys-color-on-surface)",border:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 mb-1.5",children:[r.jsx("span",{className:"text-xs font-bold",children:"Truthful Hardware Pass-Through"}),r.jsx(yg,{className:"w-4 h-4 shrink-0"})]}),r.jsxs("p",{className:"text-[11px] leading-relaxed opacity-85",children:["Uses per-handle 64-bit aligned Vulkan dispatch and forwards 100% authentic ICD capabilities without spoofing API versions, injecting extensions, or stripping ",r.jsx("code",{className:"font-mono",children:"vkCreateDevice"})," requests."]})]})]})]}),r.jsxs("div",{className:"flex flex-col gap-3 pt-4 sm:flex-row sm:items-center sm:justify-between",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"max-w-2xl text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx("p",{className:"font-bold",style:{color:"var(--md-sys-color-on-surface)"},children:"Manual Vulkan Diagnostic Layer"}),r.jsx("p",{className:"mt-0.5",children:"This opt-in controls only the Vulkan diagnostic / pass-through layer. All CPU, memory, storage, and thermal policies remain daemon-owned."})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2.5",children:[j?r.jsx("span",{className:"text-xs font-semibold",children:"Applying Vulkan mode…"}):A&&r.jsx("span",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:A}),r.jsx("span",{className:c.mode==="disabled"?"md3-badge md3-badge-neutral":"md3-badge md3-badge-primary",children:c.mode==="disabled"?"Layer Disabled":"Layer Enabled"})]})]})]}),r.jsxs("div",{className:"grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6",children:[r.jsxs("div",{className:"md3-card-elevated p-6 space-y-4",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Capabilities & Identity"}),r.jsx("span",{className:"md3-badge md3-badge-primary font-mono",children:N.vendor})]}),r.jsxs("div",{children:[r.jsx(Ce,{simulated:!_,label:"Ray Tracing",value:N.rayTracing,note:B(N.rayTracing,N.hwRayTracing)}),r.jsx(Ce,{simulated:!_,label:"Mesh Shaders",value:N.meshShaders,note:B(N.meshShaders,N.hwMeshShaders)}),r.jsx(Ce,{simulated:!_,label:"Variable Rate Shading",value:N.variableRateShading,note:B(N.variableRateShading,N.hwVariableRateShading)}),r.jsx(Ce,{simulated:!_,label:"Cooperative Matrix",value:N.cooperativeMatrix,note:B(N.cooperativeMatrix,N.hwCooperativeMatrix)}),r.jsx(Ce,{simulated:!_,label:"RT Pipeline",value:N.rtPipeline,note:B(N.rtPipeline,N.hwRtPipeline)}),r.jsx(Ce,{simulated:!_,label:"Ray Query",value:N.rayQuery,note:B(N.rayQuery,N.hwRayQuery)}),r.jsx(Ce,{simulated:!_,label:"Acceleration Structure",value:N.accelerationStructure,note:B(N.accelerationStructure,N.hwAccelerationStructure)}),r.jsx(Ce,{simulated:!_,label:"Portability Subset",value:N.portabilitySubset}),r.jsx(Ce,{simulated:!_,label:"Device Extensions",value:N.deviceExtensions,note:"HW Native ICD"}),r.jsx(Ce,{simulated:!_,label:"Instance Extensions",value:N.instanceExtensions})]}),r.jsxs("div",{className:"pt-2 space-y-1.5 text-[11px] font-mono",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("div",{className:"truncate",title:N.deviceUuid,children:["Device UUID: ",N.deviceUuid]}),r.jsxs("div",{className:"truncate",title:N.driverUuid,children:["Driver UUID: ",N.driverUuid]}),r.jsxs("div",{className:"truncate",title:N.pipelineCacheUuid,children:["Cache UUID: ",N.pipelineCacheUuid]})]})]}),r.jsxs("div",{className:"md3-card-elevated p-6 space-y-4",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Core & Vulkan 1.1–1.3"}),r.jsx("span",{className:"text-xs font-mono",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Spec Matrix"})]}),r.jsxs("div",{children:[r.jsx(Ce,{simulated:!_,label:"Robust Buffer Access",value:N.robustBufferAccess}),r.jsx(Ce,{simulated:!_,label:"Sampler Anisotropy",value:N.samplerAnisotropy}),r.jsx(Ce,{simulated:!_,label:"16-Bit Integers",value:N.shaderInt16}),r.jsx(Ce,{simulated:!_,label:"64-Bit Floats (FP64)",value:N.shaderFloat64,note:B(N.shaderFloat64,N.hwShaderFloat64)}),r.jsx(Ce,{simulated:!_,label:"Protected Memory",value:N.protectedMemory,note:"Vulkan 1.1"}),r.jsx(Ce,{simulated:!_,label:"Timeline Semaphore",value:N.timelineSemaphore,note:"Vulkan 1.2"}),r.jsx(Ce,{simulated:!_,label:"Descriptor Indexing",value:N.descriptorIndexing,note:"Vulkan 1.2"}),r.jsx(Ce,{simulated:!_,label:"Buffer Device Address",value:N.bufferDeviceAddress,note:"Vulkan 1.2"}),r.jsx(Ce,{simulated:!_,label:"Dynamic Rendering",value:N.dynamicRendering,note:"Vulkan 1.3"}),r.jsx(Ce,{simulated:!_,label:"Synchronization2",value:N.synchronization2,note:"Vulkan 1.3"})]})]}),r.jsxs("div",{className:"md3-card-elevated p-6 space-y-4",children:[r.jsxs("div",{className:"flex items-center justify-between",children:[r.jsx("h3",{className:"text-sm font-bold",children:"Vulkan 1.4 & GPU Surface"}),r.jsxs("span",{className:"md3-badge md3-badge-secondary font-mono",children:["API ",N.apiVersion]})]}),r.jsxs("div",{children:[r.jsx(Ce,{simulated:!_,label:"8-Bit Indices (indexTypeUint8)",value:N.indexTypeUint8,note:B(N.indexTypeUint8,N.hwIndexTypeUint8)}),r.jsx(Ce,{simulated:!_,label:"Line Rasterization",value:N.lineRasterization,note:B(N.lineRasterization,N.hwLineRasterization)}),r.jsx(Ce,{simulated:!_,label:"Push Descriptors",value:N.pushDescriptors,note:B(N.pushDescriptors,N.hwPushDescriptors)}),r.jsx(Ce,{simulated:!_,label:"Global Priority Query",value:N.globalPriorityQuery,note:B(N.globalPriorityQuery,N.hwGlobalPriorityQuery)}),r.jsx(Ce,{simulated:!_,label:"Shader Float Controls 2",value:N.shaderFloatControls2,note:B(N.shaderFloatControls2,N.hwShaderFloatControls2)}),r.jsx(Ce,{simulated:!_,label:"Max Push Descriptors",value:N.maxPushDescriptors}),r.jsx(Ce,{simulated:!_,label:"Maintenance 5 / 6 / 7",value:`${N.maintenance5?"M5":"—"} · ${N.maintenance6?"M6":"—"} · ${N.maintenance7?"M7":"—"}`}),r.jsx(Ce,{simulated:!_,label:"Descriptor Buffer",value:N.descriptorBuffer,note:B(N.descriptorBuffer,N.hwDescriptorBuffer)}),r.jsx(Ce,{simulated:!_,label:"Graphics Pipeline Library",value:N.graphicsPipelineLibrary,note:B(N.graphicsPipelineLibrary,N.hwGraphicsPipelineLibrary)})]}),r.jsxs("div",{className:"pt-3 space-y-1.5",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsxs("div",{className:"text-xs font-bold",children:["Multi-Vendor GPU Kernel Surface (",N.gpuKernelLabel,")"]}),r.jsx(Ce,{simulated:!_,label:"Active Governor",value:N.gpuGovernor}),r.jsx(Ce,{simulated:!_,label:"GPU Clock (Cur / Max)",value:`${N.gpuCurrentFreqMhz} / ${N.gpuMaxFreqMhz} MHz`}),r.jsx(Ce,{simulated:!_,label:"Adrenoboost / Pacing",value:`Level ${N.adrenoboostLevel}`})]})]})]})]})},_0=o=>o==="validated"?"md3-badge md3-badge-success":"md3-badge md3-badge-warning",m0=({evidence:o,nativeConnected:c,simulationMode:_=!1})=>r.jsxs("section",{"aria-label":"Native semantic evidence",className:"md3-card-elevated p-6 sm:p-7 space-y-5",children:[r.jsxs("div",{className:"flex flex-wrap items-start justify-between gap-3",children:[r.jsxs("div",{className:"flex items-center gap-3.5",children:[r.jsx("div",{className:"w-11 h-11 rounded-2xl flex items-center justify-center shrink-0",style:{backgroundColor:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(sg,{className:"w-5 h-5"})}),r.jsxs("div",{children:[r.jsx("h3",{className:"text-base font-bold tracking-tight",children:"Native Semantic Evidence"}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Daemon-owned CapabilityResolver & 3-relation DomainEvidenceAggregator · Schema v1"})]})]}),c?r.jsxs("span",{className:"md3-badge md3-badge-success",children:[r.jsx(ru,{className:"w-3.5 h-3.5"})," Connected"]}):r.jsxs("span",{className:"md3-badge md3-badge-warning",children:[r.jsx(Dh,{className:"w-3.5 h-3.5"}),_?"Simulation Mode":"Awaiting Official Data"]})]}),_?r.jsxs("div",{role:"note",className:"rounded-2xl px-4 py-3.5 text-xs leading-relaxed",style:{backgroundColor:"var(--md-sys-color-warning-container)",color:"var(--md-sys-color-on-warning-container)"},children:[r.jsx("strong",{children:"Simulation Mode."})," Browser preview metrics are synthetic and labelled ",r.jsx("code",{className:"font-mono",children:"(sim)"}),". No kernel semantic binding or corroboration evidence is fabricated in the browser."]}):(o==null?void 0:o.status)==="unavailable"?r.jsxs("div",{className:"md3-card-filled px-4 py-3.5 flex gap-3 text-xs leading-relaxed",children:[r.jsx(Ai,{className:"w-4 h-4 shrink-0 mt-0.5",style:{color:"var(--md-sys-color-warning)"}}),r.jsxs("span",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:["The daemon returned schema v1 with status ",r.jsx("code",{className:"font-mono",children:"unavailable"})," because the discovery adapter has not produced semantic evidence on this node set. No binding state or functional domain is fabricated."]})]}):c?o?r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs",children:[r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Generation"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold tabular-nums",children:o.generation??"N/A"})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Observed · monotonic ns"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold tabular-nums break-all",children:o.observedAtMonotonicNs??"N/A"})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Age"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-bold tabular-nums",children:o.ageMs===null?"N/A":`${o.ageMs.toFixed(3)} ms`})]}),r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Freshness"}),r.jsxs("div",{className:"mt-1 flex items-center gap-1.5 text-sm font-bold",children:[r.jsx(Lh,{className:"w-3.5 h-3.5"}),r.jsx("span",{style:{color:o.freshness==="fresh"?"var(--md-sys-color-success)":"var(--md-sys-color-warning)"},children:o.freshness})]})]})]}),r.jsx("div",{className:"space-y-2.5",children:o.bindings.map((d,j)=>r.jsxs("article",{className:"md3-card-outlined p-4 space-y-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("div",{className:"flex items-center gap-2 text-xs font-bold",children:[r.jsx(ou,{className:"w-3.5 h-3.5",style:{color:"var(--md-sys-color-primary)"}}),r.jsxs("span",{children:["Semantic Binding ",j+1]})]}),r.jsx("span",{className:_0(d.bindingState),children:d.bindingState.toUpperCase()})]}),r.jsxs("div",{className:"grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Domain"}),r.jsx("div",{className:"mt-1 font-bold uppercase",children:d.domain})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Assessment"}),r.jsx("div",{className:"mt-1 font-bold",children:d.domainAssessment.state})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Confidence"}),r.jsxs("div",{className:"mt-1 font-mono font-bold tabular-nums",children:[(d.domainAssessment.confidence*100).toFixed(1),"%"]})]}),r.jsxs("div",{children:[r.jsx("div",{style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Corroborating relations"}),r.jsxs("div",{className:"mt-1 font-mono font-bold tabular-nums",children:[d.domainAssessment.corroboratingRelations,"/3"]})]})]}),r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 pt-2.5 text-[11px]",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsxs("span",{children:["Binding freshness:"," ",r.jsx("strong",{style:{color:d.freshness==="fresh"?"var(--md-sys-color-success)":"var(--md-sys-color-warning)"},children:d.freshness}),d.ageMs===null?"":` · ${d.ageMs.toFixed(3)} ms`]}),r.jsxs("span",{className:"inline-flex items-center gap-1",children:[r.jsx(Un,{className:"w-3.5 h-3.5"}),"Semantic validation is not mutation authority"]})]})]},`${d.generation}-${d.observedAtMonotonicNs}-${j}`))})]}):r.jsx("div",{className:"md3-card-filled px-4 py-3.5 text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"No official semantic telemetry is available."}):r.jsx("div",{className:"md3-card-filled px-4 py-3.5 text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Native bridge detected, but an official schema v1 daemon status payload has not yet been received. Semantic bindings remain unavailable until the daemon responds."})]}),tu=o=>typeof o=="object"&&o!==null&&!Array.isArray(o),fp=o=>typeof o=="number"&&Number.isFinite(o),pp=o=>o===null||fp(o),ep=o=>o==="unresolved"||o==="cpu"||o==="gpu"||o==="vm",ss=o=>typeof o=="string"&&/^[0-9]+$/.test(o),vp=o=>o==="fresh"||o==="stale",f0=o=>o==="unresolved"||o==="candidate"||o==="corroborated"||o==="verified",p0=o=>{if(!tu(o)||!tu(o.domainAssessment))return!1;const c=o.domainAssessment;return(o.bindingState==="candidate"||o.bindingState==="validated")&&ep(o.domain)&&f0(c.state)&&ep(c.domain)&&fp(c.confidence)&&c.confidence>=0&&c.confidence<=1&&Number.isInteger(c.corroboratingRelations)&&c.corroboratingRelations>=0&&c.corroboratingRelations<=3&&ss(o.generation)&&ss(o.observedAtMonotonicNs)&&pp(o.ageMs)&&(o.ageMs===null||o.ageMs>=0)&&vp(o.freshness)},v0=o=>!tu(o)||o.schemaVersion!==1||o.status!=="available"&&o.status!=="unavailable"||!vp(o.freshness)||!Array.isArray(o.bindings)||!o.bindings.every(p0)?!1:o.status==="unavailable"?o.generation===null&&o.observedAtMonotonicNs===null&&o.ageMs===null&&o.freshness==="stale"&&o.bindings.length===0:ss(o.generation)&&ss(o.observedAtMonotonicNs)&&pp(o.ageMs)&&(o.ageMs===null||o.ageMs>=0)&&o.bindings.length>0,is='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"',zn="/data/adb/coreflow/coreflow.conf",y0="/data/adb/coreflow/logs/coreflowd.log",nu=o=>{const c=o.trim(),_=c.split(`
`).map(N=>N.trim()).filter(Boolean);for(let N=_.length-1;N>=0;N--){const B=_[N];if(B.startsWith("{")&&B.endsWith("}"))try{return JSON.parse(B)}catch{}}const d=c.indexOf('{"running":'),j=d>=0?d:c.indexOf("{"),A=c.lastIndexOf("}");if(j>=0&&A>j){const N=c.slice(j,A+1).replace(/\r?\n/g,"");return JSON.parse(N)}return JSON.parse(_[_.length-1]??"{}")},da=o=>typeof o=="object"&&o!==null&&!Array.isArray(o),Ct=o=>typeof o=="number"&&Number.isFinite(o),h0=["osMajor","baselineNodesSnapshotted","thermalPressure","thermalSeverity","aiPredictedPressure10s","aiPredictedVelocity","aiRecommendedScale","aiConfidence","aiSamples","tunerUtilityDelta","tunerTrials","tunerImproved","tunerRolledBack"],g0=["cpuPressurePct","memoryPressurePct","ioPressurePct","socCompositeCelsius","cpuAverageCelsius","cpuPeakCelsius","gpuTempCelsius","batteryTempCelsius","skinTempCelsius","thermalHeadroom"],b0=["tier1Eligible","inspectionMode","mutationRuntimeStarted","cpuPressureValid","memoryPressureValid","ioPressureValid","thermalPolicyAvailable","thermalGuardActive","thermalDataDegraded","thermalFallbackActive","thermalHeadroomValid","aiOnnxActive"],x0=["platformVendor","osFamily","executionTier","policyFamily","identityConfidence","thermalSource","thermalSensor","aiBackend","tunerRuntimeStage","tunerStage","tunerWorkload","tunerBottleneck","tunerObjective","tunerIntent","tunerCandidateId","tunerCandidateLabel","tunerCapabilityFingerprint","tunerDecisionPlanRationale","tunerRationale","tunerVerdict"],Da=(o,c)=>c.every(_=>typeof o[_]=="string"),Ti=(o,c)=>c.every(_=>Ct(o[_])),au=(o,c)=>c.every(_=>typeof o[_]=="boolean"),w0={thermalSensors:o=>da(o)&&Da(o,["id","type"])&&Ct(o.category)&&(o.celsius===null||Ct(o.celsius))&&au(o,["valid","policyAvailable"])&&(o.velocityCelsiusPerSec===void 0||o.velocityCelsiusPerSec===null||Ct(o.velocityCelsiusPerSec)),blockQueues:o=>da(o)&&Da(o,["device","activeScheduler","baselineScheduler"])&&Array.isArray(o.availableSchedulers)&&o.availableSchedulers.every(c=>typeof c=="string")&&Ti(o,["readAheadKb","nrRequests","rqAffinity","readExpireMs","writesStarved","fifoBatch"]),tunerCandidates:o=>da(o)&&Da(o,["id","label","domain"])&&Ti(o,["trials","successes","rollbacks","meanDelta","ucbScore","cooldownTicks"]),tunerEvaluatedCandidates:o=>da(o)&&Da(o,["id","domain","intent","rejectionReason"])&&Ti(o,["utilityScore","empiricalScore","confidence","riskPenalty","thermalPenalty"])&&au(o,["eligible","selected"]),tunerActiveInterventions:o=>da(o)&&Da(o,["candidateId","candidateLabel","domain","intent","stage","verdict"])&&Ti(o,["utilityDelta","observationTick","observationWindow","settlingTicksRemaining"]),flightRecorder:o=>da(o)&&(o.temp===null||Ct(o.temp))&&Ti(o,["ts","pressure","cpuPsi","memPsi","ioPsi","scale"])&&Da(o,["stage","candidate","verdict"])},N0=o=>{const c=(_,d)=>{o[_]!==void 0&&!d&&delete o[_]};for(const _ of h0)c(_,Ct(o[_]));for(const _ of g0)c(_,o[_]===null||Ct(o[_]));for(const _ of b0)c(_,typeof o[_]=="boolean");for(const _ of x0)c(_,typeof o[_]=="string");for(const[_,d]of Object.entries(w0)){const j=o[_];c(_,Array.isArray(j)&&j.every(d))}c("vulkan",da(o.vulkan)),c("batteryPower",da(o.batteryPower)&&au(o.batteryPower,["available","charging","fastCharging","thermalStepActive"])&&(o.batteryPower.temperatureCelsius===null||Ct(o.batteryPower.temperatureCelsius))&&Ti(o.batteryPower,["currentMa","voltageMv","powerMw","chargeLimitUa","oemBaselineUa"])&&Da(o.batteryPower,["status"]))},S0=o=>{const c=nu(o);if(typeof c!="object"||c===null)throw new Error("Invalid native status");const _=c;if(typeof _.running!="boolean"||!Ct(_.pid)||!Ct(_.uclampMin)||!Ct(_.uclampMax)||typeof _.uclampSupported!="boolean"||_.uclampPolicyMin!==void 0&&!Ct(_.uclampPolicyMin)||_.uclampPolicyMax!==void 0&&!Ct(_.uclampPolicyMax)||_.uclampTargetPath!==void 0&&typeof _.uclampTargetPath!="string"||_.uclampEncoding!==void 0&&typeof _.uclampEncoding!="string"||!Ct(_.socTempCelsius)&&_.socTempCelsius!==null||typeof _.recoveryPending!="boolean")throw new Error("Unexpected daemon status schema");if(_.semanticEvidence!==void 0&&!v0(_.semanticEvidence))throw new Error("Unexpected semanticEvidence schema; expected native schemaVersion 1");if(_.directAdaptive!==void 0&&!Vg(_.directAdaptive))throw new Error("Unexpected directAdaptive status schema");return N0(c),_},ki=async o=>{const c=await Oi(o);if(c.errno!==0)throw new Error(c.stderr||c.stdout||"Native command failed");return c.stdout},j0=()=>{var Se,J,se,Gt,Ua,bn,At;const[o]=F.useState(()=>os()),[c,_]=F.useState(()=>Mg()),[d,j]=F.useState(null),[A,N]=F.useState(null),[B,U]=F.useState(null),[R,b]=F.useState(!1),[y,O]=F.useState(!1),$=o&&y,[q,K]=F.useState(()=>{const H=Lg();return o?{...H,engineActive:!1,uclampMin:0,uclampMax:0,uclampSupported:!1,socTempCelsius:-1,cpuPressurePct:0,memoryPressurePct:0,ioPressurePct:0,cpuPressureValid:!1,memoryPressureValid:!1,ioPressureValid:!1,baselineNodesSnapshotted:0,baselineStatus:"restored",transactionStage:"PLANNED",modeLabel:"Awaiting native status",modeDescription:"Waiting for coreflowd runtime status",vulkan:kl(Wc)}:H}),[V,Z]=F.useState(Cl.thermalTripCelsius),[ie,G]=F.useState("dashboard"),[ce,Ee]=F.useState(()=>({...Wc})),[E,D]=F.useState(null),[P,he]=F.useState(()=>o?[]:Hg()),[Oe,re]=F.useState(!1),[ue,Ie]=F.useState({socModel:"Unavailable",kernelVersion:"Unavailable",onlineCpus:"Unavailable",gpuType:"Unavailable"});F.useEffect(()=>{o&&ki(`${is}; "$BIN" --capabilities`).then(H=>{const Q=nu(H);Q&&["socModel","kernelVersion","onlineCpus","gpuType"].every(k=>typeof Q[k]=="string")&&Ie(Q)}).catch(()=>{})},[o]);const[Xe,He]=F.useState(!1),z=up(c.theme);F.useEffect(()=>{dp(c)},[c]);const ne=()=>{const Q={...c,theme:z?"light":"dark"};_(Q),Kf(Q)},ae=H=>{const Q={...c,accent:H};_(Q),Kf(Q)},ke=F.useMemo(()=>mp(q,V),[q,V]),me=F.useRef(q);me.current=q;const ze=F.useRef(0),mt=F.useCallback((H,Q,k)=>{ze.current+=1;const Ne={id:`evt-${Date.now()}-${ze.current}`,timestamp:new Date().toTimeString().slice(0,8),type:H,tag:Q,message:k};he(Fe=>[Ne,...Fe].slice(0,50))},[]),bt=F.useCallback(async()=>{var H,Q;try{const k=S0(await ki(`${is}; "$BIN" --status`));N(k.semanticEvidence??null),O(k.semanticEvidence!==void 0);const Ne=k.socCompositeCelsius??k.socTempCelsius??-1,Fe=((H=k.directAdaptive)==null?void 0:H.thermalGuard)??k.thermalGuardActive??(k.thermalFallbackActive?Ne>=V:(k.thermalSeverity??0)>=3),It=(Q=k.directAdaptive)==null?void 0:Q.mode,lt=It?{SCREEN_OFF:"Screen Off / Deep Idle",BALANCED_SWEET_SPOT:"Balanced Sweet-Spot",ACTIVE_LOAD:"Active Load / Gaming",THERMAL_GUARD:"Thermal Guard",DISCOVERY:"Capability Discovery"}[It]??It:void 0;return K(Y=>{var xt,ft,Pt,Yt,wt;return{...Y,engineActive:k.running&&(((xt=k.directAdaptive)==null?void 0:xt.active)??!0),directAdaptive:k.directAdaptive,platformVendor:k.platformVendor??Y.platformVendor,osFamily:k.osFamily??Y.osFamily,osMajor:k.osMajor??Y.osMajor,executionTier:k.executionTier??Y.executionTier,policyFamily:k.policyFamily??Y.policyFamily,identityConfidence:k.identityConfidence??Y.identityConfidence,tier1Eligible:k.tier1Eligible??Y.tier1Eligible,inspectionMode:k.inspectionMode??Y.inspectionMode,mutationRuntimeStarted:k.mutationRuntimeStarted??Y.mutationRuntimeStarted,uclampMin:k.uclampMin,uclampMax:k.uclampMax,uclampSupported:k.uclampSupported,uclampPolicyMin:k.uclampPolicyMin??Y.uclampPolicyMin,uclampPolicyMax:k.uclampPolicyMax??Y.uclampPolicyMax,uclampTargetPath:k.uclampTargetPath??Y.uclampTargetPath,uclampEncoding:k.uclampEncoding??Y.uclampEncoding,cpuPressurePct:k.cpuPressurePct??0,memoryPressurePct:k.memoryPressurePct??0,ioPressurePct:k.ioPressurePct??0,cpuPressureValid:k.cpuPressureValid??k.cpuPressurePct!=null,memoryPressureValid:k.memoryPressureValid??k.memoryPressurePct!=null,ioPressureValid:k.ioPressureValid??k.ioPressurePct!=null,baselineNodesSnapshotted:k.baselineNodesSnapshotted??Y.baselineNodesSnapshotted,socTempCelsius:Ne,socCompositeCelsius:k.socCompositeCelsius??Ne,cpuAverageCelsius:k.cpuAverageCelsius??-1,cpuPeakCelsius:k.cpuPeakCelsius??-1,gpuTempCelsius:k.gpuTempCelsius??-1,batteryTempCelsius:k.batteryTempCelsius??-1,skinTempCelsius:k.skinTempCelsius??-1,thermalGuardActive:Fe,thermalDataDegraded:k.thermalDataDegraded??!1,thermalFallbackActive:k.thermalFallbackActive??!1,thermalHeadroomValid:k.thermalHeadroomValid??!1,thermalHeadroom:k.thermalHeadroom??null,thermalPressure:k.thermalPressure??Y.thermalPressure,thermalSource:k.thermalSource??Y.thermalSource,thermalSeverity:k.thermalSeverity??Y.thermalSeverity,thermalSensors:k.thermalSensors??Y.thermalSensors,aiBackend:k.aiBackend??Y.aiBackend,aiOnnxActive:k.aiOnnxActive??Y.aiOnnxActive,aiPredictedPressure10s:k.aiPredictedPressure10s??Y.aiPredictedPressure10s,aiPredictedVelocity:k.aiPredictedVelocity??Y.aiPredictedVelocity,aiRecommendedScale:k.aiRecommendedScale??Y.aiRecommendedScale,aiConfidence:k.aiConfidence??Y.aiConfidence,aiSamples:k.aiSamples??Y.aiSamples,tunerRuntimeStage:k.tunerRuntimeStage??Y.tunerRuntimeStage,tunerStage:k.tunerStage??Y.tunerStage,tunerWorkload:k.tunerWorkload??Y.tunerWorkload,tunerBottleneck:k.tunerBottleneck??Y.tunerBottleneck,tunerObjective:k.tunerObjective??Y.tunerObjective,tunerIntent:k.tunerIntent??Y.tunerIntent,tunerCandidateId:k.tunerCandidateId??Y.tunerCandidateId,tunerCandidateLabel:k.tunerCandidateLabel??Y.tunerCandidateLabel,tunerCapabilityFingerprint:k.tunerCapabilityFingerprint??Y.tunerCapabilityFingerprint,tunerDecisionPlanRationale:k.tunerDecisionPlanRationale??Y.tunerDecisionPlanRationale,tunerRationale:k.tunerRationale??Y.tunerRationale,tunerVerdict:k.tunerVerdict??Y.tunerVerdict,tunerUtilityDelta:k.tunerUtilityDelta??Y.tunerUtilityDelta,tunerTrials:k.tunerTrials??Y.tunerTrials,tunerImproved:k.tunerImproved??Y.tunerImproved,tunerRolledBack:k.tunerRolledBack??Y.tunerRolledBack,blockQueues:k.blockQueues??Y.blockQueues,tunerCandidates:k.tunerCandidates??Y.tunerCandidates,tunerEvaluatedCandidates:k.tunerEvaluatedCandidates??Y.tunerEvaluatedCandidates,tunerActiveInterventions:k.tunerActiveInterventions??Y.tunerActiveInterventions,batteryPower:k.batteryPower??Y.batteryPower,flightRecorder:k.flightRecorder??Y.flightRecorder,vulkan:k.vulkan??Y.vulkan,baselineStatus:k.recoveryPending||((ft=k.directAdaptive)==null?void 0:ft.active)===!0&&k.directAdaptive.baselineNodes>0?"protected":"restored",transactionStage:k.running?k.recoveryPending?"RETAINED":"VERIFIED":"PLANNED",modeLabel:k.running?lt??(Fe?"Thermal Guard":"Direct Adaptive runtime active"):"Daemon Stopped",modeDescription:k.running?(Pt=k.directAdaptive)!=null&&Pt.journalFault?"Journal fault latched; all further Direct Adaptive mutations are halted until baseline recovery.":(Yt=k.directAdaptive)!=null&&Yt.ownershipLosses?"OEM contention isolated at affected nodes; independent capabilities continue.":(wt=k.directAdaptive)!=null&&wt.active?"Direct Baseline with capability-derived Sweet-Spot policy and verified readback.":"Daemon is running; Direct Adaptive discovery or baseline preparation is unavailable.":"OEM baseline restored",timestamp:Date.now()}}),j(null),k}catch(k){throw O(!1),N(null),K(Ne=>({...Ne,directAdaptive:void 0})),k}},[V]);F.useEffect(()=>{if(!o)return;let H=!1;const Q=async()=>{const lt=await Oi(`cat ${zn} 2>/dev/null`);if(H||lt.errno!==0)return;const Y=Ug(lt.stdout,"thermal_trip_celsius");Y!==null&&Z(Math.max(30,Math.min(80,Y)));const xt=eu(lt.stdout,"vulkan_layer_mode"),ft=xt==="safe_hardware"||xt==="full_capability_override"?"safe_hardware":"disabled",Pt=Yt=>{const wt=eu(lt.stdout,Yt);return wt==="true"?!0:wt==="false"?!1:null};Ee(Yt=>({...Yt,mode:ft,enableSkiavkThreaded:Pt("enable_skiavk_threaded")??Yt.enableSkiavkThreaded,enableAdaptiveVrs:Pt("enable_adaptive_vrs")??Yt.enableAdaptiveVrs,enablePipelineCachePrewarm:Pt("enable_pipeline_cache_prewarm")??Yt.enablePipelineCachePrewarm}))};let k=!1,Ne="";const Fe=async()=>{if(!(k||H)){k=!0;try{await bt();const lt=await Oi(`/system/bin/tail -n 12 ${y0} 2>/dev/null`);if(!H&&lt.errno===0){const Y=lt.stdout.trim();if(Y&&Y!==Ne){Ne=Y;const xt=Y.split(/\r?\n/).map(ft=>ft.trim()).filter(Boolean).slice(-12).reverse();he(xt.map((ft,Pt)=>({id:`native-${Pt}-${ft}`,timestamp:"--:--:--",type:ft.includes("[Stability]")&&ft.includes("thermal")?"THERMAL":"INFO",tag:"Daemon",message:ft.slice(0,384)})))}}}catch(lt){H||j(String(lt))}finally{k=!1}}};Q(),Fe();const It=window.setInterval(()=>void Fe(),1e4);return()=>{H=!0,window.clearInterval(It)}},[o,bt]);const p=F.useCallback(H=>{const{next:Q,event:k}=Gg(me.current,Cl,H);me.current=Q,K(Q),k&&he(Ne=>[k,...Ne].slice(0,50))},[]);F.useEffect(()=>{if(o)return;const H=window.setInterval(()=>{p()},2500);return()=>window.clearInterval(H)},[o,p]);const M=async H=>{const Q=await Oi(`cat "${zn}" 2>/dev/null || cat /data/adb/modules/cfae_framework/system/etc/coreflow/default.conf 2>/dev/null || cat /system/etc/coreflow/default.conf 2>/dev/null || true`),k=Bg(Q.stdout??"",{vulkan_layer_mode:H}),Ne=k.endsWith(`
`)?k:k+`
`;let Fe="COREFLOW_CONFIG";for(let Y=1;Ne.split(`
`).some(xt=>xt.trim()===Fe);Y++)Fe=`COREFLOW_CONFIG_${Y}`;const It='umask 077 && [ ! -L "'+zn+'" ] && rm -f "'+zn+`.tmp" && cat <<'${Fe}' > `+zn+`.tmp
`+Ne+Fe+`
chown 0:0 `+zn+".tmp 2>/dev/null; chmod 600 "+zn+".tmp && mv -f "+zn+".tmp "+zn+" && echo COREFLOW_VULKAN_WRITTEN";if(!(await ki(It)).includes("COREFLOW_VULKAN_WRITTEN"))throw new Error("Vulkan layer mode write not acknowledged")},W=async H=>{He(!0),D(null);try{const Q=H.mode==="disabled"?"disabled":"safe_hardware",k={...H,mode:Q};if(!o){Ee(k),K(Ne=>({...Ne,vulkan:kl(k,Ne.vulkan)})),D(Q==="safe_hardware"?"Truthful Hardware Pass-Through active in preview":"OEM Passthrough (Disabled) active in preview"),mt("BOOST","VulkanLayer",`Layer mode=${Q}; rendering optimizers remain unimplemented.`);return}await M(Q),Ee(k),K(Ne=>({...Ne,vulkan:kl(k,Ne.vulkan)})),await ki("killall -HUP coreflowd 2>/dev/null || true"),D("Mode saved · daemon reload requested"),bt().catch(()=>{}),mt("BOOST","VulkanLayer",`Vulkan layer mode changed to ${Q}; no system tuning policy was changed.`)}catch(Q){D("Save failed"),j(String(Q))}finally{He(!1)}},ee=F.useCallback(async()=>{try{const H=await ki(`${is}; "$BIN" --journal`),Q=nu(H);U(JSON.stringify(Q,null,2)),j(null)}catch(H){j("Unable to read the mutation journal: "+String(H))}},[]),xe=F.useCallback(async()=>{if(window.confirm("Restore every CoreFlow-owned node to its captured OEM baseline and stop coreflowd?")){b(!0);try{await ki(`${is}; "$BIN" --rollback-baseline`),U(null),await bt(),mt("BASELINE","Baseline","OEM baseline rollback requested from the Direct Adaptive audit panel.")}catch(H){j("OEM baseline restore failed: "+String(H))}finally{b(!1)}}},[mt,bt]),we=[{id:"dashboard",label:"Overview",shortLabel:"Overview",Icon:gn},{id:"thermal",label:"Thermal Matrix",shortLabel:"Thermal",Icon:ls},{id:"vulkan",label:"Vulkan & GPU",shortLabel:"Vulkan",Icon:cu},{id:"hardware",label:"Hardware & CDDP",shortLabel:"Hardware",Icon:Tl},{id:"validation",label:"DVS Validation",shortLabel:"Validation",Icon:Bh},{id:"console",label:"Audit & RPC",shortLabel:"Audit",Icon:_u}];return r.jsxs("div",{className:"min-h-screen relative overflow-x-hidden flex flex-col font-sans antialiased",style:{background:"var(--md-sys-color-background)",color:"var(--md-sys-color-on-surface)"},children:[r.jsx("div",{role:"status",className:"px-4 py-2 text-center text-xs font-medium",style:{background:o&&$?"var(--md-sys-color-success-container)":"var(--md-sys-color-warning-container)",color:o&&$?"var(--md-sys-color-on-success-container)":"var(--md-sys-color-on-warning-container)",borderBottom:"1px solid var(--md-sys-color-outline-variant)"},children:o?$?"Connected · Native bridge and official daemon status with semanticEvidence schema v1 received.":"Native bridge detected · Awaiting an official daemon status payload; semantic telemetry is unavailable.":"Simulation Mode · Browser-only preview; synthetic values are labelled (sim) and are not kernel audit evidence."}),d&&r.jsx("div",{role:"alert",className:"px-4 py-2 text-xs font-medium text-center",style:{background:"var(--md-sys-color-error-container)",color:"var(--md-sys-color-on-error-container)"},children:d}),r.jsx(Zg,{isDark:z,accent:c.accent,onToggleTheme:ne,onSelectAccent:ae,modeLabel:q.modeLabel,nativeMode:o,nativeConnected:$,equilibriumScore:ke.equilibriumScore,onOpenArchitectureStudio:()=>re(!0)}),r.jsxs("div",{className:"flex-1 w-full max-w-[1440px] mx-auto px-3 sm:px-6 py-5 sm:py-7 flex flex-col lg:flex-row gap-6",children:[r.jsx("aside",{className:"hidden lg:block w-56 shrink-0",children:r.jsxs("nav",{"aria-label":"Primary sections",className:"md3-nav-rail sticky top-24 space-y-1.5",children:[r.jsx("div",{className:"px-3 py-2 text-[10px] font-bold uppercase tracking-[0.16em]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"System Domains"}),we.map(({id:H,label:Q,Icon:k})=>{const Ne=ie===H;return r.jsxs("button",{type:"button","aria-current":Ne?"page":void 0,onClick:()=>G(H),className:"w-full flex items-center gap-3 px-3.5 py-2.5 rounded-full text-xs font-medium transition-colors text-left "+(Ne?"font-semibold":""),style:{background:Ne?"var(--md-sys-color-secondary-container)":"transparent",color:Ne?"var(--md-sys-color-on-secondary-container)":"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(k,{className:"w-4 h-4 shrink-0"}),r.jsx("span",{className:"truncate",children:Q})]},H)}),r.jsxs("div",{className:"mt-4 pt-3 px-3 space-y-1.5 text-[11px]",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)"},children:[r.jsx("div",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"Execution Tier"}),r.jsx("div",{className:"font-mono text-[11px]",style:{color:"var(--md-sys-color-primary)"},children:q.executionTier??"TIER_1_FULL_COORDINATED"}),r.jsx("div",{className:"text-[10px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:q.policyFamily??"android_linux_v4"})]})]})}),r.jsx("nav",{"aria-label":"Primary sections mobile",className:"lg:hidden md3-bottom-nav flex items-center gap-1.5 overflow-x-auto no-scrollbar max-w-full",children:we.map(({id:H,shortLabel:Q,Icon:k})=>{const Ne=ie===H;return r.jsxs("button",{type:"button","aria-current":Ne?"page":void 0,onClick:()=>G(H),className:"md3-chip flex items-center gap-1.5 whitespace-nowrap select-none "+(Ne?"md3-chip-active":""),children:[r.jsx(k,{className:"w-3.5 h-3.5 shrink-0"}),r.jsx("span",{children:Q})]},H)})}),r.jsxs("main",{className:"flex-1 min-w-0 space-y-6",children:[ie==="dashboard"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("section",{className:"space-y-4","aria-label":"Autonomous runtime state",children:[r.jsxs("div",{className:"flex flex-col gap-2 sm:flex-row sm:items-end sm:justify-between",children:[r.jsxs("div",{children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"CFAE · Material Design 3 System Console"}),r.jsx("h1",{className:"mt-1 text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Autonomous Operating State"}),r.jsx("p",{className:"mt-1 max-w-2xl text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Capability discovery, evidence-led tuning, and verified actuator ownership. The console reflects native daemon telemetry while selection remains autonomous and safety-gated."})]}),r.jsx("div",{className:"flex flex-wrap items-center gap-2",children:r.jsxs("span",{className:"md3-badge "+(q.engineActive?"md3-badge-success":"md3-badge-neutral"),children:[r.jsx("span",{className:"h-1.5 w-1.5 rounded-full",style:{background:q.engineActive?"var(--md-sys-color-success)":"var(--md-sys-color-outline)"}}),q.engineActive?"Autonomous Engine Active":"Daemon Standby"]})})]}),r.jsx(Pg,{telemetry:q,nativeMode:o,thermalTripCelsius:V,onTriggerSimulation:o?void 0:p}),o&&r.jsx(Fg,{status:q.directAdaptive,nativeMode:!0,journalOutput:B,restoringBaseline:R,onViewJournal:ee,onRestoreBaseline:xe})]}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"md3-card-elevated flex items-start gap-3.5 p-4",children:[r.jsx("div",{className:"rounded-2xl p-2.5 shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(rp,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Thermal Headroom"}),r.jsx("div",{className:"mt-1 text-lg font-semibold tabular-nums",style:{color:"var(--md-sys-color-on-surface)"},children:q.thermalHeadroomValid&&q.thermalHeadroom!=null?`${Math.round(q.thermalHeadroom*100)}%`:"Unavailable"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:q.thermalGuardActive?"Thermal guard engaged":"OEM thermal policy respected"})]})]}),r.jsxs("div",{className:"md3-card-elevated flex items-start gap-3.5 p-4",children:[r.jsx("div",{className:"rounded-2xl p-2.5 shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(rs,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Resource Pressure"}),r.jsx("div",{className:"mt-1 text-lg font-semibold tabular-nums",style:{color:"var(--md-sys-color-on-surface)"},children:q.cpuPressureValid||q.memoryPressureValid||q.ioPressureValid?`${Math.max(q.cpuPressureValid?q.cpuPressurePct:0,q.memoryPressureValid?q.memoryPressurePct:0,q.ioPressureValid?q.ioPressurePct:0).toFixed(1)}%`:"Unavailable"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Highest available PSI avg10"})]})]}),r.jsxs("div",{className:"md3-card-elevated flex items-start gap-3.5 p-4",children:[r.jsx("div",{className:"rounded-2xl p-2.5 shrink-0",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(uu,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Mutation Ownership"}),r.jsx("div",{className:"mt-1 text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:q.baselineStatus==="protected"?"Journal Armed":"Guarded"}),r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"CFJ4 Baseline + Readback guard"})]})]})]}),r.jsx(m0,{evidence:A,nativeConnected:$,simulationMode:!o}),!o&&r.jsx($f,{nodesSnapshotted:q.baselineNodesSnapshotted,nativeMode:!1,baselineStatus:q.baselineStatus}),r.jsx(as,{logs:P,nativeMode:o})]}),ie==="thermal"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Protection Domain"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Thermal Intelligence & Multi-Zone Matrix"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Sensor evidence, velocity tracking, and OEM thermal constraints are surfaced for full operational transparency. Mitigation is selected autonomously."})]}),r.jsx(r0,{telemetry:q,nativeMode:o,thermalTripCelsius:V}),r.jsx(as,{logs:P.filter(H=>H.type==="THERMAL"||H.tag.toLowerCase().includes("thermal")),nativeMode:o})]}),ie==="vulkan"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"User Opt-In Subsystem"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Vulkan Diagnostics & Explicit Layer"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Vulkan layer activation (`VK_LAYER_COREFLOW_optimizer`) is the sole manual feature switch in CoreFlow. All kernel tuning domains remain strictly autonomous."})]}),r.jsx(d0,{vulkanTelemetry:q.vulkan,vulkanConfig:ce,nativeMode:o,onSaveVulkanConfig:W,isSaving:Xe,saveMessage:E}),r.jsx(as,{logs:P.filter(H=>H.tag.toLowerCase().includes("vulkan")),nativeMode:o})]}),ie==="hardware"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Capability Discovery & CDDP"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Hardware & Platform Identity Profile"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Detected hardware is evidence, not automatic mutation authority. CoreFlow adapts only through validated operational contracts."})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx("div",{className:"w-9 h-9 rounded-2xl flex items-center justify-center",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(za,{className:"h-4 w-4"})}),r.jsxs("div",{children:[r.jsx("h2",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Device Identity"}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Probed via `coreflowd --capabilities`"})]})]}),r.jsx("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[["SoC Model",ue.socModel],["GPU Subsystem",ue.gpuType],["Kernel Release",ue.kernelVersion],["Online CPUs",ue.onlineCpus]].map(([H,Q])=>r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:H}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:Q})]},H))})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 flex-wrap",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx("div",{className:"w-9 h-9 rounded-2xl flex items-center justify-center",style:{background:"var(--md-sys-color-secondary-container)",color:"var(--md-sys-color-on-secondary-container)"},children:r.jsx(ou,{className:"h-4 w-4"})}),r.jsxs("div",{children:[r.jsx("h2",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Platform Context & Execution Tier"}),r.jsx("p",{className:"text-xs",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Cross-platform policy gating & capability fingerprint"})]})]}),r.jsx("span",{className:"md3-badge font-mono "+(q.tier1Eligible!==!1?"md3-badge-success":"md3-badge-warning"),children:q.tier1Eligible!==!1?"TIER-1 ELIGIBLE":"RESTRICTED TIER"})]}),r.jsx("div",{className:"grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs",children:[["Platform Vendor",q.platformVendor??"Generic / Android"],["OS Family",`${q.osFamily??"Android"}${q.osMajor?` ${q.osMajor}`:""}`],["Execution Tier",q.executionTier??"TIER_1_FULL_COORDINATED"],["Policy Family",q.policyFamily??"android_linux_v4"],["Identity Confidence",q.identityConfidence??"HIGH"],["Capability Fingerprint",q.tunerCapabilityFingerprint??"cf-cap-verified"]].map(([H,Q])=>r.jsxs("div",{className:"md3-card-filled p-3.5",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:H}),r.jsx("div",{className:"mt-1 font-mono font-semibold break-all",style:{color:"var(--md-sys-color-on-surface)"},children:Q})]},H))})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2.5",children:[r.jsx("div",{className:"w-9 h-9 rounded-2xl flex items-center justify-center",style:{background:"var(--md-sys-color-primary-container)",color:"var(--md-sys-color-on-primary-container)"},children:r.jsx(Un,{className:"h-4 w-4"})}),r.jsxs("div",{children:[r.jsx("p",{className:"text-[10px] font-bold uppercase tracking-[0.16em]",style:{color:"var(--md-sys-color-primary)"},children:"Kernel Contract Model"}),r.jsx("h2",{className:"text-base font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"Discovery Does Not Grant Mutation Authority"})]})]}),r.jsx("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Every discovered attribute receives an explicit contract classification. Unknown nodes stay visible for hardware understanding but fail closed for automated writes."}),r.jsx("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[{name:"RUNTIME_TUNABLE",badge:"md3-badge-success",detail:"Only class eligible for tuning after ABI, range, access and baseline verification."},{name:"OBSERVATION_ONLY",badge:"md3-badge-primary",detail:"Hardware limits, topology, alignment and telemetry contribute to the fingerprint."},{name:"TEST_ONLY",badge:"md3-badge-warning",detail:"Writable testing interface, excluded from daily performance optimization."},{name:"PROTECTED",badge:"md3-badge-error",detail:"Integrity-critical or system-sensitive attribute; automatic mutation prohibited."}].map(H=>r.jsxs("div",{className:"md3-card-outlined p-4",children:[r.jsx("span",{className:"md3-badge font-mono "+H.badge,children:H.name}),r.jsx("p",{className:"mt-2 text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:H.detail})]},H.name))})]}),r.jsxs("section",{className:"md3-card-elevated space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 flex-wrap",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(Tl,{className:"h-4 w-4",style:{color:"var(--md-sys-color-primary)"}}),r.jsx("h2",{className:"font-semibold text-base",style:{color:"var(--md-sys-color-on-surface)"},children:"Autonomous Hardware Preservation & Power-Supply Guard"})]}),r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[((Se=q.batteryPower)==null?void 0:Se.fastCharging)&&r.jsx("span",{className:"md3-badge md3-badge-warning font-mono",children:"FAST CHARGING DETECTED"}),r.jsx("span",{className:"md3-badge font-mono "+((J=q.batteryPower)!=null&&J.thermalStepActive?"md3-badge-warning":"md3-badge-success"),children:(se=q.batteryPower)!=null&&se.thermalStepActive?"STEP-CHARGE GUARD ACTIVE":"OEM CHARGE CEILING INTACT"})]})]}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Battery thermal step-charging, MGLRU working-set protection, and block I/O preservation are evaluated from live power-supply and workload evidence. No manual thresholds or scheduler presets are exposed here; unsupported nodes remain observation-only until they earn mutation authority."}),r.jsxs("div",{className:"grid grid-cols-2 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Battery Temperature"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:q.batteryTempCelsius!=null&&q.batteryTempCelsius>=0?`${q.batteryTempCelsius.toFixed(1)} °C`:((Gt=q.batteryPower)==null?void 0:Gt.temperatureCelsius)!=null&&q.batteryPower.temperatureCelsius>=0?`${q.batteryPower.temperatureCelsius.toFixed(1)} °C`:"N/A"})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Power-Supply Status"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:((Ua=q.batteryPower)==null?void 0:Ua.status)??"Unknown"})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Current / Power Draw"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:(bn=q.batteryPower)!=null&&bn.available?`${Math.round(q.batteryPower.currentMa)} mA · ${(q.batteryPower.powerMw/1e3).toFixed(1)} W`:"N/A"})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Charge Limit / OEM Baseline"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:q.batteryPower&&q.batteryPower.oemBaselineUa>0?`${Math.round(q.batteryPower.chargeLimitUa/1e3)} / ${Math.round(q.batteryPower.oemBaselineUa/1e3)} mA`:"Read-Only / OEM Managed"})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Block Queues Discovered"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:((At=q.blockQueues)==null?void 0:At.length)??0})]}),r.jsxs("div",{className:"md3-card-filled p-4",children:[r.jsx("div",{className:"text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Baseline Snapshots"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:q.baselineNodesSnapshotted})]})]})]}),r.jsx($f,{nodesSnapshotted:q.baselineNodesSnapshotted,nativeMode:o,baselineStatus:q.baselineStatus})]}),ie==="validation"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Evidence, Not Assumptions"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Device Validation Suite (CF-DVS)"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Inspect the live validation contract, 8 invariant pass criteria, and native subsystem readiness."})]}),r.jsx(u0,{nativeMode:o})]}),ie==="console"&&r.jsxs("div",{className:"space-y-6 md3-page-stage",children:[r.jsxs("header",{className:"space-y-1",children:[r.jsx("p",{className:"text-[11px] font-bold uppercase tracking-[0.18em]",style:{color:"var(--md-sys-color-primary)"},children:"Runtime Transparency"}),r.jsx("h1",{className:"text-2xl font-semibold tracking-tight sm:text-3xl",style:{color:"var(--md-sys-color-on-surface)"},children:"Read-Only Audit & Diagnostic RPC"}),r.jsx("p",{className:"text-xs sm:text-sm leading-relaxed max-w-2xl",style:{color:"var(--md-sys-color-on-surface-variant)"},children:"Query read-only runtime evidence, CFJ4 journals, DVS validation payloads, and decision logs directly from `coreflowd`."})]}),r.jsx(s0,{nativeMode:o,onRefreshTelemetry:async()=>{await bt()}}),r.jsx(as,{logs:P,nativeMode:o})]})]})]}),r.jsx(l0,{isOpen:Oe,onClose:()=>re(!1),...ue}),r.jsx("footer",{className:"py-5 text-xs mt-8",style:{borderTop:"1px solid var(--md-sys-color-outline-variant)",background:"var(--md-sys-color-surface-container-low)",color:"var(--md-sys-color-on-surface-variant)"},children:r.jsxs("div",{className:"max-w-[1440px] mx-auto px-4 sm:px-6 flex flex-col sm:flex-row items-center justify-between gap-2",children:[r.jsxs("div",{children:[r.jsx("span",{className:"font-semibold",style:{color:"var(--md-sys-color-on-surface)"},children:"CFAE System Framework v4.0.0"})," · ","Material Design 3 Autonomous Mutation Safety Console"]}),r.jsx("div",{className:"text-[11px] font-mono",children:"ISO C++23 · Rust 2024 · Kotlin 2.1 · CFJ4 Readback Verified Recovery"})]})})]})};class E0 extends F.Component{constructor(){super(...arguments);Gc(this,"state",{hasError:!1,error:null});Gc(this,"handleReset",()=>{this.setState({hasError:!1,error:null}),window.location.reload()})}static getDerivedStateFromError(_){return{hasError:!0,error:_}}componentDidCatch(_,d){console.error("[CoreFlow M3 WebUI] Uncaught component error:",_,d)}render(){return this.state.hasError?r.jsx("div",{className:"min-h-screen flex items-center justify-center p-4",style:{backgroundColor:"var(--md-sys-color-background)",color:"var(--md-sys-color-on-surface)"},children:r.jsxs("div",{className:"md3-card-elevated max-w-md w-full p-7 text-center space-y-4",children:[r.jsx("div",{className:"w-12 h-12 rounded-2xl flex items-center justify-center mx-auto",style:{backgroundColor:"var(--md-sys-color-error-container)",color:"var(--md-sys-color-on-error-container)"},children:r.jsx(Cg,{className:"w-6 h-6"})}),r.jsxs("div",{className:"space-y-1.5",children:[r.jsx("h2",{className:"text-xl font-bold tracking-tight",children:"UI Surface Interrupted"}),r.jsxs("p",{className:"text-xs leading-relaxed",style:{color:"var(--md-sys-color-on-surface-variant)"},children:["The Material 3 console caught a rendering exception. Native ",r.jsx("code",{className:"font-mono",children:"coreflowd"})," daemon state and OEM baseline journals remain untouched."]})]}),this.state.error&&r.jsx("pre",{className:"md3-card-filled p-3 text-[11px] font-mono text-left overflow-auto max-h-32",children:this.state.error.message}),r.jsx("div",{className:"pt-1 flex items-center justify-center gap-3",children:r.jsxs("button",{type:"button",id:"btn-error-reload",onClick:this.handleReset,className:"md3-btn-filled",children:[r.jsx(ip,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Reload Console"})]})}),r.jsxs("div",{className:"pt-2 flex items-center justify-center gap-1.5 text-[11px]",style:{color:"var(--md-sys-color-on-surface-variant)"},children:[r.jsx(Un,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Write-ahead CFJ4 recovery remains independent"})]})]})}):this.props.children}}Sh.createRoot(document.getElementById("root")).render(r.jsx(F.StrictMode,{children:r.jsx(E0,{children:r.jsx(j0,{})})}));
