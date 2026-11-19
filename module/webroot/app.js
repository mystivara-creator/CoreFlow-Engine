var ch=Object.defineProperty;var uh=(o,c,m)=>c in o?ch(o,c,{enumerable:!0,configurable:!0,writable:!0,value:m}):o[c]=m;var Vc=(o,c,m)=>uh(o,typeof c!="symbol"?c+"":c,m);(function(){const c=document.createElement("link").relList;if(c&&c.supports&&c.supports("modulepreload"))return;for(const S of document.querySelectorAll('link[rel="modulepreload"]'))d(S);new MutationObserver(S=>{for(const q of S)if(q.type==="childList")for(const k of q.addedNodes)k.tagName==="LINK"&&k.rel==="modulepreload"&&d(k)}).observe(document,{childList:!0,subtree:!0});function m(S){const q={};return S.integrity&&(q.integrity=S.integrity),S.referrerPolicy&&(q.referrerPolicy=S.referrerPolicy),S.crossOrigin==="use-credentials"?q.credentials="include":S.crossOrigin==="anonymous"?q.credentials="omit":q.credentials="same-origin",q}function d(S){if(S.ep)return;S.ep=!0;const q=m(S);fetch(S.href,q)}})();var Lc={exports:{}},vl={};/**
 * @license React
 * react-jsx-runtime.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Mf;function dh(){if(Mf)return vl;Mf=1;var o=Symbol.for("react.transitional.element"),c=Symbol.for("react.fragment");function m(d,S,q){var k=null;if(q!==void 0&&(k=""+q),S.key!==void 0&&(k=""+S.key),"key"in S){q={};for(var M in S)M!=="key"&&(q[M]=S[M])}else q=S;return S=q.ref,{$$typeof:o,type:d,key:k,ref:S!==void 0?S:null,props:q}}return vl.Fragment=c,vl.jsx=m,vl.jsxs=m,vl}var Uf;function _h(){return Uf||(Uf=1,Lc.exports=dh()),Lc.exports}var r=_h(),Hc={exports:{}},le={};/**
 * @license React
 * react.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Bf;function mh(){if(Bf)return le;Bf=1;var o=Symbol.for("react.transitional.element"),c=Symbol.for("react.portal"),m=Symbol.for("react.fragment"),d=Symbol.for("react.strict_mode"),S=Symbol.for("react.profiler"),q=Symbol.for("react.consumer"),k=Symbol.for("react.context"),M=Symbol.for("react.forward_ref"),O=Symbol.for("react.suspense"),T=Symbol.for("react.memo"),C=Symbol.for("react.lazy"),y=Symbol.for("react.activity"),z=Symbol.for("react.view_transition"),Y=Symbol.iterator;function L(p){return p===null||typeof p!="object"?null:(p=Y&&p[Y]||p["@@iterator"],typeof p=="function"?p:null)}var I={isMounted:function(){return!1},enqueueForceUpdate:function(){},enqueueReplaceState:function(){},enqueueSetState:function(){}},J=Object.assign,K={};function ae(p,A,W){this.props=p,this.context=A,this.refs=K,this.updater=W||I}ae.prototype.isReactComponent={},ae.prototype.setState=function(p,A){if(typeof p!="object"&&typeof p!="function"&&p!=null)throw Error("takes an object of state variables to update or a function which returns an object of state variables.");this.updater.enqueueSetState(this,p,A,"setState")},ae.prototype.forceUpdate=function(p){this.updater.enqueueForceUpdate(this,p,"forceUpdate")};function Z(){}Z.prototype=ae.prototype;function _e(p,A,W){this.props=p,this.context=A,this.refs=K,this.updater=W||I}var Se=_e.prototype=new Z;Se.constructor=_e,J(Se,ae.prototype),Se.isPureReactComponent=!0;var V=Array.isArray;function H(){}var ne={H:null,A:null,T:null,S:null},De=Object.prototype.hasOwnProperty;function Be(p,A,W){var $=W.ref;return{$$typeof:o,type:p,key:A,ref:$!==void 0?$:null,props:W}}function Me(p,A){return Be(p.type,A,p.props)}function we(p){return typeof p=="object"&&p!==null&&p.$$typeof===o}function Je(p){var A={"=":"=0",":":"=2"};return"$"+p.replace(/[=:]/g,function(W){return A[W]})}var it=/\/+/g;function Ve(p,A){return typeof p=="object"&&p!==null&&p.key!=null?Je(""+p.key):A.toString(36)}function D(p){switch(p.status){case"fulfilled":return p.value;case"rejected":throw p.reason;default:switch(typeof p.status=="string"?p.then(H,H):(p.status="pending",p.then(function(A){p.status==="pending"&&(p.status="fulfilled",p.value=A)},function(A){p.status==="pending"&&(p.status="rejected",p.reason=A)})),p.status){case"fulfilled":return p.value;case"rejected":throw p.reason}}throw p}function te(p,A,W,$,me){var he=typeof p;(he==="undefined"||he==="boolean")&&(p=null);var U=!1;if(p===null)U=!0;else switch(he){case"bigint":case"string":case"number":U=!0;break;case"object":switch(p.$$typeof){case o:case c:U=!0;break;case C:return U=p._init,te(U(p._payload),A,W,$,me)}}if(U)return me=me(p),U=$===""?"."+Ve(p,0):$,V(me)?(W="",U!=null&&(W=U.replace(it,"$&/")+"/"),te(me,A,W,"",function(ge){return ge})):me!=null&&(we(me)&&(me=Me(me,W+(me.key==null||p&&p.key===me.key?"":(""+me.key).replace(it,"$&/")+"/")+U)),A.push(me)),1;U=0;var R=$===""?".":$+":";if(V(p))for(var N=0;N<p.length;N++)$=p[N],he=R+Ve($,N),U+=te($,A,W,he,me);else if(N=L(p),typeof N=="function")for(p=N.call(p),N=0;!($=p.next()).done;)$=$.value,he=R+Ve($,N++),U+=te($,A,W,he,me);else if(he==="object"){if(typeof p.then=="function")return te(D(p),A,W,$,me);throw A=String(p),Error("Objects are not valid as a React child (found: "+(A==="[object Object]"?"object with keys {"+Object.keys(p).join(", ")+"}":A)+"). If you meant to render a collection of children, use an array instead.")}return U}function Q(p,A,W){if(p==null)return p;var $=[],me=0;return te(p,$,"","",function(he){return A.call(W,he,me++)}),$}function ke(p){if(p._status===-1){var A=p._result,W=A();W.then(function($){(p._status===0||p._status===-1)&&(p._status=1,p._result=$,W.status===void 0&&(W.status="fulfilled",W.value=$))},function($){(p._status===0||p._status===-1)&&(p._status=2,p._result=$,W.status===void 0&&(W.status="rejected",W.reason=$))}),p._status===-1&&(p._status=0,p._result=W)}if(p._status===1)return p._result.default;throw p._result}var ve=typeof reportError=="function"?reportError:function(p){if(typeof window=="object"&&typeof window.ErrorEvent=="function"){var A=new window.ErrorEvent("error",{bubbles:!0,cancelable:!0,message:typeof p=="object"&&p!==null&&typeof p.message=="string"?String(p.message):String(p),error:p});if(!window.dispatchEvent(A))return}else if(typeof process=="object"&&typeof process.emit=="function"){process.emit("uncaughtException",p);return}console.error(p)};function We(p){var A=ne.T,W={};W.types=A!==null?A.types:null,ne.T=W;try{var $=p(),me=ne.S;me!==null&&me(W,$),typeof $=="object"&&$!==null&&typeof $.then=="function"&&$.then(H,ve)}catch(he){ve(he)}finally{A!==null&&W.types!==null&&(A.types=W.types),ne.T=A}}function Ut(p){var A=ne.T;if(A!==null){var W=A.types;W===null?A.types=[p]:W.indexOf(p)===-1&&W.push(p)}else We(Ut.bind(null,p))}var mn={map:Q,forEach:function(p,A,W){Q(p,function(){A.apply(this,arguments)},W)},count:function(p){var A=0;return Q(p,function(){A++}),A},toArray:function(p){return Q(p,function(A){return A})||[]},only:function(p){if(!we(p))throw Error("React.Children.only expected to receive a single React element child.");return p}};return le.Activity=y,le.Children=mn,le.Component=ae,le.Fragment=m,le.Profiler=S,le.PureComponent=_e,le.StrictMode=d,le.Suspense=O,le.ViewTransition=z,le.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE=ne,le.__COMPILER_RUNTIME={__proto__:null,c:function(p){return ne.H.useMemoCache(p)}},le.addTransitionType=Ut,le.cache=function(p){return function(){return p.apply(null,arguments)}},le.cacheSignal=function(){return null},le.cloneElement=function(p,A,W){if(p==null)throw Error("The argument must be a React element, but you passed "+p+".");var $=J({},p.props),me=p.key;if(A!=null)for(he in A.key!==void 0&&(me=""+A.key),A)!De.call(A,he)||he==="key"||he==="__self"||he==="__source"||he==="ref"&&A.ref===void 0||($[he]=A[he]);var he=arguments.length-2;if(he===1)$.children=W;else if(1<he){for(var U=Array(he),R=0;R<he;R++)U[R]=arguments[R+2];$.children=U}return Be(p.type,me,$)},le.createContext=function(p){return p={$$typeof:k,_currentValue:p,_currentValue2:p,_threadCount:0,Provider:null,Consumer:null},p.Provider=p,p.Consumer={$$typeof:q,_context:p},p},le.createElement=function(p,A,W){var $,me={},he=null;if(A!=null)for($ in A.key!==void 0&&(he=""+A.key),A)De.call(A,$)&&$!=="key"&&$!=="__self"&&$!=="__source"&&(me[$]=A[$]);var U=arguments.length-2;if(U===1)me.children=W;else if(1<U){for(var R=Array(U),N=0;N<U;N++)R[N]=arguments[N+2];me.children=R}if(p&&p.defaultProps)for($ in U=p.defaultProps,U)me[$]===void 0&&(me[$]=U[$]);return Be(p,he,me)},le.createRef=function(){return{current:null}},le.forwardRef=function(p){return{$$typeof:M,render:p}},le.isValidElement=we,le.lazy=function(p){return{$$typeof:C,_payload:{_status:-1,_result:p},_init:ke}},le.memo=function(p,A){return{$$typeof:T,type:p,compare:A===void 0?null:A}},le.startTransition=We,le.unstable_useCacheRefresh=function(){return ne.H.useCacheRefresh()},le.use=function(p){return ne.H.use(p)},le.useActionState=function(p,A,W){return ne.H.useActionState(p,A,W)},le.useCallback=function(p,A){return ne.H.useCallback(p,A)},le.useContext=function(p){return ne.H.useContext(p)},le.useDebugValue=function(){},le.useDeferredValue=function(p,A){return ne.H.useDeferredValue(p,A)},le.useEffect=function(p,A){return ne.H.useEffect(p,A)},le.useEffectEvent=function(p){return ne.H.useEffectEvent(p)},le.useId=function(){return ne.H.useId()},le.useImperativeHandle=function(p,A,W){return ne.H.useImperativeHandle(p,A,W)},le.useInsertionEffect=function(p,A){return ne.H.useInsertionEffect(p,A)},le.useLayoutEffect=function(p,A){return ne.H.useLayoutEffect(p,A)},le.useMemo=function(p,A){return ne.H.useMemo(p,A)},le.useOptimistic=function(p,A){return ne.H.useOptimistic(p,A)},le.useReducer=function(p,A,W){return ne.H.useReducer(p,A,W)},le.useRef=function(p){return ne.H.useRef(p)},le.useState=function(p){return ne.H.useState(p)},le.useSyncExternalStore=function(p,A,W){return ne.H.useSyncExternalStore(p,A,W)},le.useTransition=function(){return ne.H.useTransition()},le.version="19.3.1",le}var Vf;function au(){return Vf||(Vf=1,Hc.exports=mh()),Hc.exports}var P=au(),Gc={exports:{}},hl={},Yc={exports:{}},Ic={};/**
 * @license React
 * scheduler.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Lf;function fh(){return Lf||(Lf=1,(function(o){function c(D,te){var Q=D.length;D.push(te);e:for(;0<Q;){var ke=Q-1>>>1,ve=D[ke];if(0<S(ve,te))D[ke]=te,D[Q]=ve,Q=ke;else break e}}function m(D){return D.length===0?null:D[0]}function d(D){if(D.length===0)return null;var te=D[0],Q=D.pop();if(Q!==te){D[0]=Q;e:for(var ke=0,ve=D.length,We=ve>>>1;ke<We;){var Ut=2*(ke+1)-1,mn=D[Ut],p=Ut+1,A=D[p];if(0>S(mn,Q))p<ve&&0>S(A,mn)?(D[ke]=A,D[p]=Q,ke=p):(D[ke]=mn,D[Ut]=Q,ke=Ut);else if(p<ve&&0>S(A,Q))D[ke]=A,D[p]=Q,ke=p;else break e}}return te}function S(D,te){var Q=D.sortIndex-te.sortIndex;return Q!==0?Q:D.id-te.id}if(o.unstable_now=void 0,typeof performance=="object"&&typeof performance.now=="function"){var q=performance;o.unstable_now=function(){return q.now()}}else{var k=Date,M=k.now();o.unstable_now=function(){return k.now()-M}}var O=[],T=[],C=1,y=null,z=3,Y=!1,L=!1,I=!1,J=!1,K=typeof setTimeout=="function"?setTimeout:null,ae=typeof clearTimeout=="function"?clearTimeout:null,Z=typeof setImmediate<"u"?setImmediate:null;function _e(D){for(var te=m(T);te!==null;){if(te.callback===null)d(T);else if(te.startTime<=D)d(T),te.sortIndex=te.expirationTime,c(O,te);else break;te=m(T)}}function Se(D){if(I=!1,_e(D),!L)if(m(O)!==null)L=!0,V||(V=!0,we());else{var te=m(T);te!==null&&Ve(Se,te.startTime-D)}}var V=!1,H=-1,ne=5,De=-1;function Be(){return J?!0:!(o.unstable_now()-De<ne)}function Me(){if(J=!1,V){var D=o.unstable_now();De=D;var te=!0;try{e:{L=!1,I&&(I=!1,ae(H),H=-1),Y=!0;var Q=z;try{t:{for(_e(D),y=m(O);y!==null&&!(y.expirationTime>D&&Be());){var ke=y.callback;if(typeof ke=="function"){y.callback=null,z=y.priorityLevel;var ve=ke(y.expirationTime<=D);if(D=o.unstable_now(),typeof ve=="function"){y.callback=ve,_e(D),te=!0;break t}y===m(O)&&d(O),_e(D)}else d(O);y=m(O)}if(y!==null)te=!0;else{var We=m(T);We!==null&&Ve(Se,We.startTime-D),te=!1}}break e}finally{y=null,z=Q,Y=!1}te=void 0}}finally{te?we():V=!1}}}var we;if(typeof Z=="function")we=function(){Z(Me)};else if(typeof MessageChannel<"u"){var Je=new MessageChannel,it=Je.port2;Je.port1.onmessage=Me,we=function(){it.postMessage(null)}}else we=function(){K(Me,0)};function Ve(D,te){H=K(function(){D(o.unstable_now())},te)}o.unstable_IdlePriority=5,o.unstable_ImmediatePriority=1,o.unstable_LowPriority=4,o.unstable_NormalPriority=3,o.unstable_Profiling=null,o.unstable_UserBlockingPriority=2,o.unstable_cancelCallback=function(D){D.callback=null},o.unstable_forceFrameRate=function(D){0>D||125<D?console.error("forceFrameRate takes a positive int between 0 and 125, forcing frame rates higher than 125 fps is not supported"):ne=0<D?Math.floor(1e3/D):5},o.unstable_getCurrentPriorityLevel=function(){return z},o.unstable_next=function(D){switch(z){case 1:case 2:case 3:var te=3;break;default:te=z}var Q=z;z=te;try{return D()}finally{z=Q}},o.unstable_requestPaint=function(){J=!0},o.unstable_runWithPriority=function(D,te){switch(D){case 1:case 2:case 3:case 4:case 5:break;default:D=3}var Q=z;z=D;try{return te()}finally{z=Q}},o.unstable_scheduleCallback=function(D,te,Q){var ke=o.unstable_now();switch(typeof Q=="object"&&Q!==null?(Q=Q.delay,Q=typeof Q=="number"&&0<Q?ke+Q:ke):Q=ke,D){case 1:var ve=-1;break;case 2:ve=250;break;case 5:ve=1073741823;break;case 4:ve=1e4;break;default:ve=5e3}return ve=Q+ve,D={id:C++,callback:te,priorityLevel:D,startTime:Q,expirationTime:ve,sortIndex:-1},Q>ke?(D.sortIndex=Q,c(T,D),m(O)===null&&D===m(T)&&(I?(ae(H),H=-1):I=!0,Ve(Se,Q-ke))):(D.sortIndex=ve,c(O,D),L||Y||(L=!0,V||(V=!0,we()))),D},o.unstable_shouldYield=Be,o.unstable_wrapCallback=function(D){var te=z;return function(){var Q=z;z=te;try{return D.apply(this,arguments)}finally{z=Q}}}})(Ic)),Ic}var Hf;function ph(){return Hf||(Hf=1,Yc.exports=fh()),Yc.exports}var Qc={exports:{}},dt={};/**
 * @license React
 * react-dom.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var Gf;function vh(){if(Gf)return dt;Gf=1;var o=au();function c(C){var y="https://react.dev/errors/"+C;if(1<arguments.length){y+="?args[]="+encodeURIComponent(arguments[1]);for(var z=2;z<arguments.length;z++)y+="&args[]="+encodeURIComponent(arguments[z])}return"Minified React error #"+C+"; visit "+y+" for the full message or use the non-minified dev environment for full errors and additional helpful warnings."}function m(){}var d={d:{f:m,r:function(){throw Error(c(522))},D:m,C:m,L:m,m,X:m,S:m,M:m},p:0,findDOMNode:null},S=Symbol.for("react.portal"),q=Symbol.for("react.recoverable"),k=Symbol.for("react.optimistic_key");function M(C,y,z){var Y=3<arguments.length&&arguments[3]!==void 0?arguments[3]:null;return{$$typeof:S,key:Y==null?null:Y===k?k:""+Y,children:C,containerInfo:y,implementation:z}}var O=o.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE;function T(C,y){if(C==="font")return"";if(typeof y=="string")return y==="use-credentials"?y:""}return dt.__DOM_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE=d,dt.browser=function(C){return{$$typeof:q,_reason:C}},dt.createPortal=function(C,y){var z=2<arguments.length&&arguments[2]!==void 0?arguments[2]:null;if(!y||y.nodeType!==1&&y.nodeType!==9&&y.nodeType!==11)throw Error(c(299));return M(C,y,null,z)},dt.flushSync=function(C){var y=O.T,z=d.p;try{if(O.T=null,d.p=2,C)return C()}finally{O.T=y,d.p=z,d.d.f()}},dt.preconnect=function(C,y){typeof C=="string"&&(y?(y=y.crossOrigin,y=typeof y=="string"?y==="use-credentials"?y:"":void 0):y=null,d.d.C(C,y))},dt.prefetchDNS=function(C){typeof C=="string"&&d.d.D(C)},dt.preinit=function(C,y){if(typeof C=="string"&&y&&typeof y.as=="string"){var z=y.as,Y=T(z,y.crossOrigin),L=typeof y.integrity=="string"?y.integrity:void 0,I=typeof y.fetchPriority=="string"?y.fetchPriority:void 0;z==="style"?d.d.S(C,typeof y.precedence=="string"?y.precedence:void 0,{crossOrigin:Y,integrity:L,fetchPriority:I}):z==="script"&&d.d.X(C,{crossOrigin:Y,integrity:L,fetchPriority:I,nonce:typeof y.nonce=="string"?y.nonce:void 0})}},dt.preinitModule=function(C,y){if(typeof C=="string")if(typeof y=="object"&&y!==null){if(y.as==null||y.as==="script"){var z=T(y.as,y.crossOrigin);d.d.M(C,{crossOrigin:z,integrity:typeof y.integrity=="string"?y.integrity:void 0,nonce:typeof y.nonce=="string"?y.nonce:void 0,fetchPriority:typeof y.fetchPriority=="string"?y.fetchPriority:void 0})}}else y==null&&d.d.M(C)},dt.preload=function(C,y){if(typeof C=="string"&&typeof y=="object"&&y!==null&&typeof y.as=="string"){var z=y.as,Y=T(z,y.crossOrigin);d.d.L(C,z,{crossOrigin:Y,integrity:typeof y.integrity=="string"?y.integrity:void 0,nonce:typeof y.nonce=="string"?y.nonce:void 0,type:typeof y.type=="string"?y.type:void 0,fetchPriority:typeof y.fetchPriority=="string"?y.fetchPriority:void 0,referrerPolicy:typeof y.referrerPolicy=="string"?y.referrerPolicy:void 0,imageSrcSet:typeof y.imageSrcSet=="string"?y.imageSrcSet:void 0,imageSizes:typeof y.imageSizes=="string"?y.imageSizes:void 0,media:typeof y.media=="string"?y.media:void 0})}},dt.preloadModule=function(C,y){if(typeof C=="string")if(y){var z=T(y.as,y.crossOrigin);d.d.m(C,{as:typeof y.as=="string"&&y.as!=="script"?y.as:void 0,crossOrigin:z,integrity:typeof y.integrity=="string"?y.integrity:void 0,nonce:typeof y.nonce=="string"?y.nonce:void 0,fetchPriority:typeof y.fetchPriority=="string"?y.fetchPriority:void 0})}else d.d.m(C)},dt.requestFormReset=function(C){d.d.r(C)},dt.unstable_batchedUpdates=function(C,y){return C(y)},dt.useFormState=function(C,y,z){return O.H.useFormState(C,y,z)},dt.useFormStatus=function(){return O.H.useHostTransitionStatus()},dt.version="19.3.1",dt}var Yf;function hh(){if(Yf)return Qc.exports;Yf=1;function o(){if(!(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__>"u"||typeof __REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE!="function"))try{__REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE(o)}catch(c){console.error(c)}}return o(),Qc.exports=vh(),Qc.exports}/**
 * @license React
 * react-dom-client.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var If;function yh(){if(If)return hl;If=1;var o=ph(),c=au(),m=hh();function d(e){var t="https://react.dev/errors/"+e;if(1<arguments.length){t+="?args[]="+encodeURIComponent(arguments[1]);for(var n=2;n<arguments.length;n++)t+="&args[]="+encodeURIComponent(arguments[n])}return"Minified React error #"+e+"; visit "+t+" for the full message or use the non-minified dev environment for full errors and additional helpful warnings."}function S(e){return!(!e||e.nodeType!==1&&e.nodeType!==9&&e.nodeType!==11)}function q(e){for(var t=e,n=t;n&&!n.alternate;)t=n,(t.flags&4098)!==0&&(e=t.return),n=t.return;for(;t.return;)t=t.return;return t.tag===3?e:null}function k(e){if(e.tag===13){var t=e.memoizedState;if(t===null&&(e=e.alternate,e!==null&&(t=e.memoizedState)),t!==null)return t.dehydrated}return null}function M(e){if(e.tag===31){var t=e.memoizedState;if(t===null&&(e=e.alternate,e!==null&&(t=e.memoizedState)),t!==null)return t.dehydrated}return null}function O(e){if(q(e)!==e)throw Error(d(188))}function T(e){var t=e.alternate;if(!t){if(t=q(e),t===null)throw Error(d(188));return t!==e?null:e}for(var n=e,a=t;;){var i=n.return;if(i===null)break;var l=i.alternate;if(l===null){if(a=i.return,a!==null){n=a;continue}break}if(i.child===l.child){for(l=i.child;l;){if(l===n)return O(i),e;if(l===a)return O(i),t;l=l.sibling}throw Error(d(188))}if(n.return!==a.return)n=i,a=l;else{for(var s=!1,u=i.child;u;){if(u===n){s=!0,n=i,a=l;break}if(u===a){s=!0,a=i,n=l;break}u=u.sibling}if(!s){for(u=l.child;u;){if(u===n){s=!0,n=l,a=i;break}if(u===a){s=!0,a=l,n=i;break}u=u.sibling}if(!s)throw Error(d(189))}}if(n.alternate!==a)throw Error(d(190))}if(n.tag!==3)throw Error(d(188));return n.stateNode.current===n?e:t}function C(e){var t=e.tag;if(t===5||t===26||t===27||t===6)return e;for(e=e.child;e!==null;){if(t=C(e),t!==null)return t;e=e.sibling}return null}function y(e,t,n,a,i,l){for(;e!==null;){if((e.tag===5||e.tag===27||e.tag===6)&&n(e,a,i,l)||(e.tag!==22||e.memoizedState===null)&&(t||e.tag!==5&&e.tag!==27)&&y(e.child,t,n,a,i,l))return!0;e=e.sibling}return!1}function z(e){for(e=e.return;e!==null;){if(e.tag===3||e.tag===5||e.tag===27)return e;e=e.return}return null}function Y(e){var t=!1;for(e=e.return;e!==null&&(e.tag===4&&(t=!0),!(e.tag===3||e.tag===5||e.tag===27));)e=e.return;return t}function L(e){var t=[null,null],n=z(e);return n===null||I(t,e,n.child,{foundSelf:!1}),t}function I(e,t,n,a){for(;n!==null;){if(n===t)a.foundSelf=!0;else if(n.tag===5||n.tag===27||n.tag===6){if(a.foundSelf)return e[1]=n,!0;e[0]=n}else if((n.tag!==22||n.memoizedState===null)&&I(e,t,n.child,a))return!0;n=n.sibling}return!1}function J(e){switch(e.tag){case 5:case 27:case 6:return e.stateNode;case 3:return e.stateNode.containerInfo;default:throw Error(d(559))}}var K=null,ae=null;function Z(e,t,n){return e===n?!0:e===t?(K=e,!0):!1}function _e(e,t,n){return e===n?(ae=e,!1):e===t?(ae!==null&&(K=e),!0):!1}function Se(e){if(e===null)return null;do e=e===null?null:e.return;while(e&&e.tag!==5&&e.tag!==27&&e.tag!==3);return e||null}function V(e,t,n){for(var a=0,i=e;i;i=n(i))a++;i=0;for(var l=t;l;l=n(l))i++;for(;0<a-i;)e=n(e),a--;for(;0<i-a;)t=n(t),i--;for(;a--;){if(e===t||t!==null&&e===t.alternate)return e;e=n(e),t=n(t)}return null}var H=Object.assign,ne=Symbol.for("react.element"),De=Symbol.for("react.transitional.element"),Be=Symbol.for("react.portal"),Me=Symbol.for("react.fragment"),we=Symbol.for("react.strict_mode"),Je=Symbol.for("react.profiler"),it=Symbol.for("react.consumer"),Ve=Symbol.for("react.context"),D=Symbol.for("react.forward_ref"),te=Symbol.for("react.suspense"),Q=Symbol.for("react.suspense_list"),ke=Symbol.for("react.memo"),ve=Symbol.for("react.lazy"),We=Symbol.for("react.activity"),Ut=Symbol.for("react.legacy_hidden"),mn=Symbol.for("react.memo_cache_sentinel"),p=Symbol.for("react.view_transition"),A=Symbol.for("react.recoverable"),W=Symbol.iterator;function $(e){return e===null||typeof e!="object"?null:(e=W&&e[W]||e["@@iterator"],typeof e=="function"?e:null)}var me=Symbol.for("react.client.reference");function he(e){if(e==null)return null;if(typeof e=="function")return e.$$typeof===me?null:e.displayName||e.name||null;if(typeof e=="string")return e;switch(e){case Me:return"Fragment";case Je:return"Profiler";case we:return"StrictMode";case te:return"Suspense";case Q:return"SuspenseList";case We:return"Activity";case p:return"ViewTransition"}if(typeof e=="object")switch(e.$$typeof){case Be:return"Portal";case Ve:return e.displayName||"Context";case it:return(e._context.displayName||"Context")+".Consumer";case D:var t=e.render;return e=e.displayName,e||(e=t.displayName||t.name||"",e=e!==""?"ForwardRef("+e+")":"ForwardRef"),e;case ke:return t=e.displayName||null,t!==null?t:he(e.type)||"Memo";case ve:t=e._payload,e=e._init;try{return he(e(t))}catch{}}return null}var U=Array.isArray,R=c.__CLIENT_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE,N=m.__DOM_INTERNALS_DO_NOT_USE_OR_WARN_USERS_THEY_CANNOT_UPGRADE,ge={pending:!1,data:null,method:null,action:null},_t=[],yt=-1;function Le(e){return{current:e}}function F(e){0>yt||(e.current=_t[yt],_t[yt]=null,yt--)}function ce(e,t){yt++,_t[yt]=e.current,e.current=t}var Ge=Le(null),gt=Le(null),$e=Le(null),en=Le(null);function bl(e,t){switch(ce($e,t),ce(gt,e),ce(Ge,null),t.nodeType){case 9:case 11:e=(e=t.documentElement)&&(e=e.namespaceURI)?Im(e):0;break;default:if(e=t.tagName,t=t.namespaceURI)t=Im(t),e=Qm(t,e);else switch(e){case"svg":e=1;break;case"math":e=2;break;default:e=0}}F(Ge),ce(Ge,e)}function Ta(){F(Ge),F(gt),F($e)}function is(e){var t=e.memoizedState;t!==null&&(pi._currentValue=t.memoizedState,ce(en,e)),t=Ge.current;var n=Qm(t,e.type);t!==n&&(ce(gt,e),ce(Ge,n))}function wl(e){gt.current===e&&(F(Ge),F(gt)),en.current===e&&(F(en),pi._currentValue=ge)}var ls,du;function On(e){if(ls===void 0)try{throw Error()}catch(n){var t=n.stack.trim().match(/\n( *(at )?)/);ls=t&&t[1]||"",du=-1<n.stack.indexOf(`
    at`)?" (<anonymous>)":-1<n.stack.indexOf("@")?"@unknown:0:0":""}return`
`+ls+e+du}var rs=!1;function ss(e,t){if(!e||rs)return"";rs=!0;var n=Error.prepareStackTrace;Error.prepareStackTrace=void 0;try{var a={DetermineComponentFrameRoot:function(){try{if(t){var j=function(){throw Error()};if(Object.defineProperty(j.prototype,"props",{set:function(){throw Error()}}),typeof Reflect=="object"&&Reflect.construct){try{Reflect.construct(j,[])}catch(B){var v=B}Reflect.construct(e,[],j)}else{try{j.call()}catch(B){v=B}j=!1;try{var b=Object.getOwnPropertyDescriptor(e.prototype,"props");Object.defineProperty(e.prototype,"props",{configurable:!0,set:function(){throw Error()}}),j=!0,new e}finally{j&&(b!==void 0?Object.defineProperty(e.prototype,"props",b):delete e.prototype.props)}}}else{try{throw Error()}catch(B){v=B}(j=e())&&typeof j.catch=="function"&&j.catch(function(){})}}catch(B){if(B&&v&&typeof B.stack=="string")return[B.stack,v.stack]}return[null,null]}};a.DetermineComponentFrameRoot.displayName="DetermineComponentFrameRoot";var i=Object.getOwnPropertyDescriptor(a.DetermineComponentFrameRoot,"name");i&&i.configurable&&Object.defineProperty(a.DetermineComponentFrameRoot,"name",{value:"DetermineComponentFrameRoot"});var l=a.DetermineComponentFrameRoot(),s=l[0],u=l[1];if(s&&u){var _=s.split(`
`),g=u.split(`
`);for(i=a=0;a<_.length&&!_[a].includes("DetermineComponentFrameRoot");)a++;for(;i<g.length&&!g[i].includes("DetermineComponentFrameRoot");)i++;if(a===_.length||i===g.length)for(a=_.length-1,i=g.length-1;1<=a&&0<=i&&_[a]!==g[i];)i--;for(;1<=a&&0<=i;a--,i--)if(_[a]!==g[i]){if(a!==1||i!==1)do if(a--,i--,0>i||_[a]!==g[i]){var w=`
`+_[a].replace(" at new "," at ");return e.displayName&&w.includes("<anonymous>")&&(w=w.replace("<anonymous>",e.displayName)),w}while(1<=a&&0<=i);break}}}finally{rs=!1,Error.prepareStackTrace=n}return(n=e?e.displayName||e.name:"")?On(n):""}function mp(e,t){switch(e.tag){case 26:case 27:case 5:return On(e.type);case 16:return On("Lazy");case 13:return e.child!==t&&t!==null?On("Suspense Fallback"):On("Suspense");case 19:return On("SuspenseList");case 0:case 15:return ss(e.type,!1);case 11:return ss(e.type.render,!1);case 1:return ss(e.type,!0);case 31:return On("Activity");case 30:return On("ViewTransition");default:return""}}function _u(e){try{var t="",n=null;do t+=mp(e,n),n=e,e=e.return;while(e);return t}catch(a){return`
Error generating stack: `+a.message+`
`+a.stack}}var os=Object.prototype.hasOwnProperty,cs=o.unstable_scheduleCallback,us=o.unstable_cancelCallback,fp=o.unstable_shouldYield,pp=o.unstable_requestPaint,Et=o.unstable_now,vp=o.unstable_getCurrentPriorityLevel,mu=o.unstable_ImmediatePriority,fu=o.unstable_UserBlockingPriority,kl=o.unstable_NormalPriority,hp=o.unstable_LowPriority,pu=o.unstable_IdlePriority,yp=o.log,gp=o.unstable_setDisableYieldValue,ki=null,jt=null;function An(e){if(typeof yp=="function"&&gp(e),jt&&typeof jt.setStrictMode=="function")try{jt.setStrictMode(ki,e)}catch{}}var Tt=Math.clz32?Math.clz32:wp,xp=Math.log,bp=Math.LN2;function wp(e){return e>>>=0,e===0?32:31-(xp(e)/bp|0)|0}var Nl=256,Sl=262144,El=4194304;function ia(e){var t=e&42;if(t!==0)return t;switch(e&-e){case 1:return 1;case 2:return 2;case 4:return 4;case 8:return 8;case 16:return 16;case 32:return 32;case 64:return 64;case 128:return 128;case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:return e&-e;case 262144:case 524288:case 1048576:case 2097152:return e&3932160;case 4194304:case 8388608:case 16777216:case 33554432:return e&62914560;case 67108864:return 67108864;case 134217728:return 134217728;case 268435456:return 268435456;case 536870912:return 536870912;case 1073741824:return 0;default:return e}}function jl(e,t,n){var a=e.pendingLanes;if(a===0)return 0;var i=0,l=e.suspendedLanes,s=e.pingedLanes;e=e.warmLanes;var u=a&134217727;return u!==0?(a=u&~l,a!==0?i=ia(a):(s&=u,s!==0?i=ia(s):n||(n=u&~e,n!==0&&(i=ia(n))))):(u=a&~l,u!==0?i=ia(u):s!==0?i=ia(s):n||(n=a&~e,n!==0&&(i=ia(n)))),i===0?0:t!==0&&t!==i&&(t&l)===0&&(l=i&-i,n=t&-t,l>=n||l===32&&(n&4194048)!==0)?t:i}function Ni(e,t){return(e.pendingLanes&~(e.suspendedLanes&~e.pingedLanes)&t)===0}function vu(e,t){(t&8)!==0&&(t|=t&32);var n=e.entangledLanes;if(n!==0)for(e=e.entanglements,n&=t;0<n;){var a=31-Tt(n),i=1<<a;t|=e[a],n&=~i}return t}function kp(e,t){switch(e){case 1:case 2:case 4:case 8:case 64:return t+250;case 16:case 32:case 128:case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:case 262144:case 524288:case 1048576:case 2097152:return t+5e3;case 4194304:case 8388608:case 16777216:case 33554432:return-1;case 67108864:case 134217728:case 268435456:case 536870912:case 1073741824:return-1;default:return-1}}function hu(){var e=El;return El<<=1,(El&62914560)===0&&(El=4194304),e}function ds(e){for(var t=[],n=0;31>n;n++)t.push(e);return t}function Si(e,t){e.pendingLanes|=t,t!==268435456&&(e.suspendedLanes=0,e.pingedLanes=0,e.warmLanes=0)}function Np(e,t,n,a,i,l){var s=e.pendingLanes;e.pendingLanes=n,e.suspendedLanes=0,e.pingedLanes=0,e.warmLanes=0,e.expiredLanes&=n,e.entangledLanes&=n,e.errorRecoveryDisabledLanes&=n,e.shellSuspendCounter=0;var u=e.entanglements,_=e.expirationTimes,g=e.hiddenUpdates;for(n=s&~n;0<n;){var w=31-Tt(n),j=1<<w;u[w]=0,_[w]=-1;var v=g[w];if(v!==null)for(g[w]=null,w=0;w<v.length;w++){var b=v[w];b!==null&&(b.lane&=-536870913)}n&=~j}a!==0&&yu(e,a,0),l!==0&&i===0&&e.tag!==0&&(e.suspendedLanes|=l&~(s&~t))}function yu(e,t,n){e.pendingLanes|=t,e.suspendedLanes&=~t;var a=31-Tt(t);e.entangledLanes|=t,e.entanglements[a]=e.entanglements[a]|1073741824|n&261930}function gu(e,t){var n=e.entangledLanes|=t;for(e=e.entanglements;n;){var a=31-Tt(n),i=1<<a;i&t|e[a]&t&&(e[a]|=t),n&=~i}}function xu(e,t){var n=t&-t;return n=(n&42)!==0?1:_s(n),(n&(e.suspendedLanes|t))!==0?0:n}function _s(e){switch(e){case 2:e=1;break;case 8:e=4;break;case 32:e=16;break;case 256:case 512:case 1024:case 2048:case 4096:case 8192:case 16384:case 32768:case 65536:case 131072:case 262144:case 524288:case 1048576:case 2097152:case 4194304:case 8388608:case 16777216:case 33554432:e=128;break;case 268435456:e=134217728;break;default:e=0}return e}function ms(e){return e&=-e,2<e?8<e?(e&134217727)!==0?32:268435456:8:2}function bu(){var e=N.p;return e!==0?e:(e=window.event,e===void 0?32:Cf(e.type))}function wu(e,t){var n=N.p;try{return N.p=e,t()}finally{N.p=n}}var fn=Math.random().toString(36).slice(2),lt="__reactFiber$"+fn,xt="__reactProps$"+fn,Ca="__reactContainer$"+fn,ku="__reactEvents$"+fn,Sp="__reactListeners$"+fn,Ep="__reactHandles$"+fn,Nu="__reactResources$"+fn,Ei="__reactMarker$"+fn,Tl="__reactLoad$"+fn;function Cl(e){delete e[lt],delete e[xt],delete e[Sp],delete e[Ep]}function la(e){var t;if(t=e[lt])return t;for(var n=e.parentNode;n;){if(t=n[Ca]||n[lt]){if(n=t.alternate,t.child!==null||n!==null&&n.child!==null)for(e=cf(e);e!==null;){if(n=e[lt])return n;e=cf(e)}return t}e=n,n=e.parentNode}return null}function Oa(e){if(e=e[lt]||e[Ca]){var t=e.tag;if(t===5||t===6||t===13||t===31||t===26||t===27||t===3)return e}return null}function ji(e){var t=e.tag;if(t===5||t===26||t===27||t===6)return e.stateNode;throw Error(d(33))}function Aa(e){var t=e[Nu];return t||(t=e[Nu]={hoistableStyles:new Map,hoistableScripts:new Map}),t}function et(e){e[Ei]=!0}function Su(e){e[Tl]=void 0}var Eu=new Set,ju={};function ra(e,t){qa(e,t),qa(e+"Capture",t)}function qa(e,t){for(ju[e]=t,e=0;e<t.length;e++)Eu.add(t[e])}var jp=RegExp("^[:A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD][:A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD\\-.0-9\\u00B7\\u0300-\\u036F\\u203F-\\u2040]*$"),Tu={},Cu={};function Tp(e){return os.call(Cu,e)?!0:os.call(Tu,e)?!1:jp.test(e)?Cu[e]=!0:(Tu[e]=!0,!1)}var Ne=!1;function Ou(){var e=Ne;return Ne=!1,e}function Ol(e,t,n){if(Tp(t))if(n===null)e.removeAttribute(t);else{switch(typeof n){case"undefined":case"function":case"symbol":e.removeAttribute(t);return;case"boolean":var a=t.toLowerCase().slice(0,5);if(a!=="data-"&&a!=="aria-"){e.removeAttribute(t);return}}e.setAttribute(t,n)}}function Al(e,t,n){if(n===null)e.removeAttribute(t);else{switch(typeof n){case"undefined":case"function":case"symbol":case"boolean":e.removeAttribute(t);return}e.setAttribute(t,n)}}function pn(e,t,n,a){if(a===null)e.removeAttribute(n);else{switch(typeof a){case"undefined":case"function":case"symbol":case"boolean":e.removeAttribute(n);return}e.setAttributeNS(t,n,a)}}function Ct(e){switch(typeof e){case"bigint":case"boolean":case"number":case"string":case"undefined":return e;case"object":return e;default:return""}}function Au(e){var t=e.type;return(e=e.nodeName)&&e.toLowerCase()==="input"&&(t==="checkbox"||t==="radio")}function Cp(e,t,n){var a=Object.getOwnPropertyDescriptor(e.constructor.prototype,t);if(!e.hasOwnProperty(t)&&typeof a<"u"&&typeof a.get=="function"&&typeof a.set=="function"){var i=a.get,l=a.set;return Object.defineProperty(e,t,{configurable:!0,get:function(){return i.call(this)},set:function(s){n=""+s,l.call(this,s)}}),Object.defineProperty(e,t,{enumerable:a.enumerable}),{getValue:function(){return n},setValue:function(s){n=""+s},stopTracking:function(){e._valueTracker=null,delete e[t]}}}}function fs(e){if(!e._valueTracker){var t=Au(e)?"checked":"value";e._valueTracker=Cp(e,t,""+e[t])}}function qu(e){if(!e)return!1;var t=e._valueTracker;if(!t)return!0;var n=t.getValue(),a="";return e&&(a=Au(e)?e.checked?"true":"false":e.value),e=a,e!==n?(t.setValue(e),!0):!1}var Op=/[\n"\\]/g;function Bt(e){return e.replace(Op,function(t){return"\\"+t.charCodeAt(0).toString(16)+" "})}function ps(e,t,n,a,i,l,s,u){e.name="",s!=null&&typeof s!="function"&&typeof s!="symbol"&&typeof s!="boolean"?e.type=s:e.removeAttribute("type"),t!=null?s==="number"?(t===0&&e.value===""||e.value!=t)&&(e.value=""+Ct(t)):e.value!==""+Ct(t)&&(e.value=""+Ct(t)):s!=="submit"&&s!=="reset"||e.removeAttribute("value"),t!=null?s==="number"&&e.value==t?vs(e,Ct(e.value)):vs(e,Ct(t)):n!=null?vs(e,Ct(n)):a!=null&&e.removeAttribute("value"),i==null&&l!=null&&(e.defaultChecked=!!l),i!=null&&(e.checked=i&&typeof i!="function"&&typeof i!="symbol"),u!=null&&typeof u!="function"&&typeof u!="symbol"&&typeof u!="boolean"?e.name=""+Ct(u):e.removeAttribute("name")}function Ru(e,t,n,a,i,l,s,u){if(l!=null&&typeof l!="function"&&typeof l!="symbol"&&typeof l!="boolean"&&(e.type=l),t!=null||n!=null){if(!(l!=="submit"&&l!=="reset"||t!=null)){fs(e);return}n=n!=null?""+Ct(n):"",t=t!=null?""+Ct(t):n,u||t===e.value||(e.value=t),e.defaultValue=t}a=a??i,a=typeof a!="function"&&typeof a!="symbol"&&!!a,e.checked=u?e.checked:!!a,e.defaultChecked=!!a,s!=null&&typeof s!="function"&&typeof s!="symbol"&&typeof s!="boolean"&&(e.name=s),fs(e)}function vs(e,t){e.defaultValue!==""+t&&(e.defaultValue=""+t)}function Ra(e,t,n,a){if(e=e.options,t){t={};for(var i=0;i<n.length;i++)t["$"+n[i]]=!0;for(n=0;n<e.length;n++)i=t.hasOwnProperty("$"+e[n].value),e[n].selected!==i&&(e[n].selected=i),i&&a&&(e[n].defaultSelected=!0)}else{for(n=""+Ct(n),t=null,i=0;i<e.length;i++){if(e[i].value===n){e[i].selected=!0,a&&(e[i].defaultSelected=!0);return}t!==null||e[i].disabled||(t=e[i])}t!==null&&(t.selected=!0)}}function zu(e,t,n){if(t!=null&&(t=""+Ct(t),t!==e.value&&(e.value=t),n==null)){e.defaultValue!==t&&(e.defaultValue=t);return}e.defaultValue=n!=null?""+Ct(n):""}function Du(e,t,n,a){if(t==null){if(a!=null){if(n!=null)throw Error(d(92));if(U(a)){if(1<a.length)throw Error(d(93));a=a[0]}n=a}n==null&&(n=""),t=n}n=Ct(t),e.defaultValue=n,a=e.textContent,a===n&&a!==""&&a!==null&&(e.value=a),fs(e)}function za(e,t){if(t){var n=e.firstChild;if(n&&n===e.lastChild&&n.nodeType===3){n.nodeValue=t;return}}e.textContent=t}var Ap=new Set("animationIterationCount aspectRatio borderImageOutset borderImageSlice borderImageWidth boxFlex boxFlexGroup boxOrdinalGroup columnCount columns flex flexGrow flexPositive flexShrink flexNegative flexOrder gridArea gridRow gridRowEnd gridRowSpan gridRowStart gridColumn gridColumnEnd gridColumnSpan gridColumnStart fontWeight lineClamp lineHeight opacity order orphans scale tabSize widows zIndex zoom fillOpacity floodOpacity stopOpacity strokeDasharray strokeDashoffset strokeMiterlimit strokeOpacity strokeWidth MozAnimationIterationCount MozBoxFlex MozBoxFlexGroup MozLineClamp msAnimationIterationCount msFlex msZoom msFlexGrow msFlexNegative msFlexOrder msFlexPositive msFlexShrink msGridColumn msGridColumnSpan msGridRow msGridRowSpan WebkitAnimationIterationCount WebkitBoxFlex WebKitBoxFlexGroup WebkitBoxOrdinalGroup WebkitColumnCount WebkitColumns WebkitFlex WebkitFlexGrow WebkitFlexPositive WebkitFlexShrink WebkitLineClamp".split(" "));function Mu(e,t,n){var a=t.indexOf("--")===0;n==null||typeof n=="boolean"||n===""?a?e.setProperty(t,""):t==="float"?e.cssFloat="":e[t]="":a?e.setProperty(t,n):typeof n!="number"||n===0||Ap.has(t)?t==="float"?e.cssFloat=n:e[t]=(""+n).trim():e[t]=n+"px"}function Uu(e,t,n){if(t!=null&&typeof t!="object")throw Error(d(62));if(e=e.style,n!=null){for(var a in n)!n.hasOwnProperty(a)||t!=null&&t.hasOwnProperty(a)||(a.indexOf("--")===0?e.setProperty(a,""):a==="float"?e.cssFloat="":e[a]="",Ne=!0);for(var i in t)a=t[i],t.hasOwnProperty(i)&&n[i]!==a&&(Mu(e,i,a),Ne=!0)}else for(var l in t)t.hasOwnProperty(l)&&Mu(e,l,t[l])}function hs(e){if(e.indexOf("-")===-1)return!1;switch(e){case"annotation-xml":case"color-profile":case"font-face":case"font-face-src":case"font-face-uri":case"font-face-format":case"font-face-name":case"missing-glyph":return!1;default:return!0}}var qp=new Map([["acceptCharset","accept-charset"],["htmlFor","for"],["httpEquiv","http-equiv"],["crossOrigin","crossorigin"],["accentHeight","accent-height"],["alignmentBaseline","alignment-baseline"],["arabicForm","arabic-form"],["baselineShift","baseline-shift"],["capHeight","cap-height"],["clipPath","clip-path"],["clipRule","clip-rule"],["colorInterpolation","color-interpolation"],["colorInterpolationFilters","color-interpolation-filters"],["colorProfile","color-profile"],["colorRendering","color-rendering"],["dominantBaseline","dominant-baseline"],["enableBackground","enable-background"],["fillOpacity","fill-opacity"],["fillRule","fill-rule"],["floodColor","flood-color"],["floodOpacity","flood-opacity"],["fontFamily","font-family"],["fontSize","font-size"],["fontSizeAdjust","font-size-adjust"],["fontStretch","font-stretch"],["fontStyle","font-style"],["fontVariant","font-variant"],["fontWeight","font-weight"],["glyphName","glyph-name"],["glyphOrientationHorizontal","glyph-orientation-horizontal"],["glyphOrientationVertical","glyph-orientation-vertical"],["horizAdvX","horiz-adv-x"],["horizOriginX","horiz-origin-x"],["imageRendering","image-rendering"],["letterSpacing","letter-spacing"],["lightingColor","lighting-color"],["markerEnd","marker-end"],["markerMid","marker-mid"],["markerStart","marker-start"],["maskType","mask-type"],["overlinePosition","overline-position"],["overlineThickness","overline-thickness"],["paintOrder","paint-order"],["panose-1","panose-1"],["pointerEvents","pointer-events"],["renderingIntent","rendering-intent"],["shapeRendering","shape-rendering"],["stopColor","stop-color"],["stopOpacity","stop-opacity"],["strikethroughPosition","strikethrough-position"],["strikethroughThickness","strikethrough-thickness"],["strokeDasharray","stroke-dasharray"],["strokeDashoffset","stroke-dashoffset"],["strokeLinecap","stroke-linecap"],["strokeLinejoin","stroke-linejoin"],["strokeMiterlimit","stroke-miterlimit"],["strokeOpacity","stroke-opacity"],["strokeWidth","stroke-width"],["textAnchor","text-anchor"],["textDecoration","text-decoration"],["textRendering","text-rendering"],["transformOrigin","transform-origin"],["underlinePosition","underline-position"],["underlineThickness","underline-thickness"],["unicodeBidi","unicode-bidi"],["unicodeRange","unicode-range"],["unitsPerEm","units-per-em"],["vAlphabetic","v-alphabetic"],["vHanging","v-hanging"],["vIdeographic","v-ideographic"],["vMathematical","v-mathematical"],["vectorEffect","vector-effect"],["vertAdvY","vert-adv-y"],["vertOriginX","vert-origin-x"],["vertOriginY","vert-origin-y"],["wordSpacing","word-spacing"],["writingMode","writing-mode"],["xmlnsXlink","xmlns:xlink"],["xHeight","x-height"]]),Rp=/^[\u0000-\u001F ]*j[\r\n\t]*a[\r\n\t]*v[\r\n\t]*a[\r\n\t]*s[\r\n\t]*c[\r\n\t]*r[\r\n\t]*i[\r\n\t]*p[\r\n\t]*t[\r\n\t]*:/i;function ql(e){return Rp.test(""+e)?"javascript:throw new Error('React has blocked a javascript: URL as a security precaution.')":e}function tn(){}var ys=null;function gs(e){return e=e.target||e.srcElement||window,e.correspondingUseElement&&(e=e.correspondingUseElement),e.nodeType===3?e.parentNode:e}var Da=null,Ma=null;function Bu(e){var t=Oa(e);if(t&&(e=t.stateNode)){var n=e[xt]||null;e:switch(e=t.stateNode,t.type){case"input":if(ps(e,n.value,n.defaultValue,n.defaultValue,n.checked,n.defaultChecked,n.type,n.name),t=n.name,n.type==="radio"&&t!=null){for(n=e;n.parentNode;)n=n.parentNode;for(n=n.querySelectorAll('input[name="'+Bt(""+t)+'"][type="radio"]'),t=0;t<n.length;t++){var a=n[t];if(a!==e&&a.form===e.form){var i=a[xt]||null;if(!i)throw Error(d(90));ps(a,i.value,i.defaultValue,i.defaultValue,i.checked,i.defaultChecked,i.type,i.name)}}for(t=0;t<n.length;t++)a=n[t],a.form===e.form&&qu(a)}break e;case"textarea":zu(e,n.value,n.defaultValue);break e;case"select":t=n.value,t!=null&&Ra(e,!!n.multiple,t,!1)}}}var xs=!1;function Vu(e,t,n){if(xs)return e(t,n);xs=!0;try{var a=e(t);return a}finally{if(xs=!1,(Da!==null||Ma!==null)&&(qr(),Da&&(t=Da,e=Ma,Ma=Da=null,Bu(t),e)))for(t=0;t<e.length;t++)Bu(e[t])}}function Ti(e,t){var n=e.stateNode;if(n===null)return null;var a=n[xt]||null;if(a===null)return null;n=a[t];e:switch(t){case"onClick":case"onClickCapture":case"onDoubleClick":case"onDoubleClickCapture":case"onMouseDown":case"onMouseDownCapture":case"onMouseMove":case"onMouseMoveCapture":case"onMouseUp":case"onMouseUpCapture":case"onMouseEnter":(a=!a.disabled)||(e=e.type,a=!(e==="button"||e==="input"||e==="select"||e==="textarea")),e=!a;break e;default:e=!1}if(e)return null;if(n&&typeof n!="function")throw Error(d(231,t,typeof n));return n}var vn=!(typeof window>"u"||typeof window.document>"u"||typeof window.document.createElement>"u"),bs=!1;if(vn)try{var Ci={};Object.defineProperty(Ci,"passive",{get:function(){bs=!0}}),window.addEventListener("test",Ci,Ci),window.removeEventListener("test",Ci,Ci)}catch{bs=!1}var qn=null,ws=null,Rl=null;function Lu(){if(Rl)return Rl;var e,t=ws,n=t.length,a,i="value"in qn?qn.value:qn.textContent,l=i.length;for(e=0;e<n&&t[e]===i[e];e++);var s=n-e;for(a=1;a<=s&&t[n-a]===i[l-a];a++);return Rl=i.slice(e,1<a?1-a:void 0)}function zl(e){var t=e.keyCode;return"charCode"in e?(e=e.charCode,e===0&&t===13&&(e=13)):e=t,e===10&&(e=13),32<=e||e===13?e:0}function Dl(){return!0}function Hu(){return!1}function ft(e){function t(n,a,i,l,s){this._reactName=n,this._targetInst=i,this.type=a,this.nativeEvent=l,this.target=s,this.currentTarget=null;for(var u in e)e.hasOwnProperty(u)&&(n=e[u],this[u]=n?n(l):l[u]);return this.isDefaultPrevented=(l.defaultPrevented!=null?l.defaultPrevented:l.returnValue===!1)?Dl:Hu,this.isPropagationStopped=Hu,this}return H(t.prototype,{preventDefault:function(){this.defaultPrevented=!0;var n=this.nativeEvent;n&&(n.preventDefault?n.preventDefault():typeof n.returnValue!="unknown"&&(n.returnValue=!1),this.isDefaultPrevented=Dl)},stopPropagation:function(){var n=this.nativeEvent;n&&(n.stopPropagation?n.stopPropagation():typeof n.cancelBubble!="unknown"&&(n.cancelBubble=!0),this.isPropagationStopped=Dl)},persist:function(){},isPersistent:Dl}),t}var Rn={eventPhase:0,bubbles:0,cancelable:0,timeStamp:function(e){return e.timeStamp||Date.now()},defaultPrevented:0,isTrusted:0},Ml=ft(Rn),Oi=H({},Rn,{view:0,detail:0}),zp=ft(Oi),ks,Ns,Ai,Ul=H({},Oi,{screenX:0,screenY:0,clientX:0,clientY:0,pageX:0,pageY:0,ctrlKey:0,shiftKey:0,altKey:0,metaKey:0,getModifierState:Es,button:0,buttons:0,relatedTarget:function(e){return e.relatedTarget===void 0?e.fromElement===e.srcElement?e.toElement:e.fromElement:e.relatedTarget},movementX:function(e){return"movementX"in e?e.movementX:(e!==Ai&&(Ai&&e.type==="mousemove"?(ks=e.screenX-Ai.screenX,Ns=e.screenY-Ai.screenY):Ns=ks=0,Ai=e),ks)},movementY:function(e){return"movementY"in e?e.movementY:Ns}}),Gu=ft(Ul),Dp=H({},Ul,{dataTransfer:0}),Mp=ft(Dp),Up=H({},Oi,{relatedTarget:0}),Ss=ft(Up),Bp=H({},Rn,{animationName:0,elapsedTime:0,pseudoElement:0}),Vp=ft(Bp),Lp=H({},Rn,{clipboardData:function(e){return"clipboardData"in e?e.clipboardData:window.clipboardData}}),Hp=ft(Lp),Gp=H({},Rn,{data:0}),Yu=ft(Gp),Yp={Esc:"Escape",Spacebar:" ",Left:"ArrowLeft",Up:"ArrowUp",Right:"ArrowRight",Down:"ArrowDown",Del:"Delete",Win:"OS",Menu:"ContextMenu",Apps:"ContextMenu",Scroll:"ScrollLock",MozPrintableKey:"Unidentified"},Ip={8:"Backspace",9:"Tab",12:"Clear",13:"Enter",16:"Shift",17:"Control",18:"Alt",19:"Pause",20:"CapsLock",27:"Escape",32:" ",33:"PageUp",34:"PageDown",35:"End",36:"Home",37:"ArrowLeft",38:"ArrowUp",39:"ArrowRight",40:"ArrowDown",45:"Insert",46:"Delete",112:"F1",113:"F2",114:"F3",115:"F4",116:"F5",117:"F6",118:"F7",119:"F8",120:"F9",121:"F10",122:"F11",123:"F12",144:"NumLock",145:"ScrollLock",224:"Meta"},Qp={Alt:"altKey",Control:"ctrlKey",Meta:"metaKey",Shift:"shiftKey"};function Kp(e){var t=this.nativeEvent;return t.getModifierState?t.getModifierState(e):(e=Qp[e])?!!t[e]:!1}function Es(){return Kp}var Xp=H({},Oi,{key:function(e){if(e.key){var t=Yp[e.key]||e.key;if(t!=="Unidentified")return t}return e.type==="keypress"?(e=zl(e),e===13?"Enter":String.fromCharCode(e)):e.type==="keydown"||e.type==="keyup"?Ip[e.keyCode]||"Unidentified":""},code:0,location:0,ctrlKey:0,shiftKey:0,altKey:0,metaKey:0,repeat:0,locale:0,getModifierState:Es,charCode:function(e){return e.type==="keypress"?zl(e):0},keyCode:function(e){return e.type==="keydown"||e.type==="keyup"?e.keyCode:0},which:function(e){return e.type==="keypress"?zl(e):e.type==="keydown"||e.type==="keyup"?e.keyCode:0}}),Zp=ft(Xp),Pp=H({},Ul,{pointerId:0,width:0,height:0,pressure:0,tangentialPressure:0,tiltX:0,tiltY:0,twist:0,pointerType:0,isPrimary:0}),Iu=ft(Pp),Fp=H({},Rn,{submitter:0}),Jp=ft(Fp),Wp=H({},Oi,{touches:0,targetTouches:0,changedTouches:0,altKey:0,metaKey:0,ctrlKey:0,shiftKey:0,getModifierState:Es}),$p=ft(Wp),e0=H({},Rn,{propertyName:0,elapsedTime:0,pseudoElement:0}),t0=ft(e0),n0=H({},Ul,{deltaX:function(e){return"deltaX"in e?e.deltaX:"wheelDeltaX"in e?-e.wheelDeltaX:0},deltaY:function(e){return"deltaY"in e?e.deltaY:"wheelDeltaY"in e?-e.wheelDeltaY:"wheelDelta"in e?-e.wheelDelta:0},deltaZ:0,deltaMode:0}),a0=ft(n0),i0=H({},Rn,{newState:0,oldState:0,source:0}),l0=ft(i0),r0=[9,13,27,32],js=vn&&"CompositionEvent"in window,qi=null;vn&&"documentMode"in document&&(qi=document.documentMode);var s0=vn&&"TextEvent"in window&&!qi,Qu=vn&&(!js||qi&&8<qi&&11>=qi),Ku=" ",Xu=!1;function Zu(e,t){switch(e){case"keyup":return r0.indexOf(t.keyCode)!==-1;case"keydown":return t.keyCode!==229;case"keypress":case"mousedown":case"focusout":return!0;default:return!1}}function Pu(e){return e=e.detail,typeof e=="object"&&"data"in e?e.data:null}var Ua=!1;function o0(e,t){switch(e){case"compositionend":return Pu(t);case"keypress":return t.which!==32?null:(Xu=!0,Ku);case"textInput":return e=t.data,e===Ku&&Xu?null:e;default:return null}}function c0(e,t){if(Ua)return e==="compositionend"||!js&&Zu(e,t)?(e=Lu(),Rl=ws=qn=null,Ua=!1,e):null;switch(e){case"paste":return null;case"keypress":if(!(t.ctrlKey||t.altKey||t.metaKey)||t.ctrlKey&&t.altKey){if(t.char&&1<t.char.length)return t.char;if(t.which)return String.fromCharCode(t.which)}return null;case"compositionend":return Qu&&t.locale!=="ko"?null:t.data;default:return null}}var u0={color:!0,date:!0,datetime:!0,"datetime-local":!0,email:!0,month:!0,number:!0,password:!0,range:!0,search:!0,tel:!0,text:!0,time:!0,url:!0,week:!0};function Fu(e){var t=e&&e.nodeName&&e.nodeName.toLowerCase();return t==="input"?!!u0[e.type]:t==="textarea"}function Ju(e,t,n,a){Da?Ma?Ma.push(a):Ma=[a]:Da=a,t=Br(t,"onChange"),0<t.length&&(n=new Ml("onChange","change",null,n,a),e.push({event:n,listeners:t}))}var Ri=null,zi=null;function d0(e){Bm(e,0)}function Bl(e){var t=ji(e);if(qu(t))return e}function Wu(e,t){if(e==="change")return t}var $u=!1;if(vn){var Ts;if(vn){var Cs="oninput"in document;if(!Cs){var ed=document.createElement("div");ed.setAttribute("oninput","return;"),Cs=typeof ed.oninput=="function"}Ts=Cs}else Ts=!1;$u=Ts&&(!document.documentMode||9<document.documentMode)}function td(){Ri&&(Ri.detachEvent("onpropertychange",nd),zi=Ri=null)}function nd(e){if(e.propertyName==="value"&&Bl(zi)){var t=[];Ju(t,zi,e,gs(e)),Vu(d0,t)}}function _0(e,t,n){e==="focusin"?(td(),Ri=t,zi=n,Ri.attachEvent("onpropertychange",nd)):e==="focusout"&&td()}function m0(e){if(e==="selectionchange"||e==="keyup"||e==="keydown")return Bl(zi)}function f0(e,t){if(e==="click")return Bl(t)}function p0(e,t){if(e==="input"||e==="change")return Bl(t)}function v0(e,t){return e===t&&(e!==0||1/e===1/t)||e!==e&&t!==t}var Ot=typeof Object.is=="function"?Object.is:v0;function Di(e,t){if(Ot(e,t))return!0;if(typeof e!="object"||e===null||typeof t!="object"||t===null)return!1;var n=Object.keys(e),a=Object.keys(t);if(n.length!==a.length)return!1;for(a=0;a<n.length;a++){var i=n[a];if(!os.call(t,i)||!Ot(e[i],t[i]))return!1}return!0}function Os(e){if(e=e||(typeof document<"u"?document:void 0),typeof e>"u")return null;try{return e.activeElement||e.body}catch{return e.body}}function ad(e){for(;e&&e.firstChild;)e=e.firstChild;return e}function id(e,t){var n=ad(e);e=0;for(var a;n;){if(n.nodeType===3){if(a=e+n.textContent.length,e<=t&&a>=t)return{node:n,offset:t-e};e=a}e:{for(;n;){if(n.nextSibling){n=n.nextSibling;break e}n=n.parentNode}n=void 0}n=ad(n)}}function ld(e,t){return e&&t?e===t?!0:e&&e.nodeType===3?!1:t&&t.nodeType===3?ld(e,t.parentNode):"contains"in e?e.contains(t):e.compareDocumentPosition?!!(e.compareDocumentPosition(t)&16):!1:!1}function rd(e){e=e!=null&&e.ownerDocument!=null&&e.ownerDocument.defaultView!=null?e.ownerDocument.defaultView:window;for(var t=Os(e.document);t instanceof e.HTMLIFrameElement;){try{var n=typeof t.contentWindow.location.href=="string"}catch{n=!1}if(n)e=t.contentWindow;else break;t=Os(e.document)}return t}function As(e){var t=e&&e.nodeName&&e.nodeName.toLowerCase();return t&&(t==="input"&&(e.type==="text"||e.type==="search"||e.type==="tel"||e.type==="url"||e.type==="password")||t==="textarea"||e.contentEditable==="true")}var h0=vn&&"documentMode"in document&&11>=document.documentMode,Ba=null,qs=null,Mi=null,Rs=!1;function sd(e,t,n){var a=n.window===n?n.document:n.nodeType===9?n:n.ownerDocument;Rs||Ba==null||Ba!==Os(a)||(a=Ba,"selectionStart"in a&&As(a)?a={start:a.selectionStart,end:a.selectionEnd}:(a=(a.ownerDocument&&a.ownerDocument.defaultView||window).getSelection(),a={anchorNode:a.anchorNode,anchorOffset:a.anchorOffset,focusNode:a.focusNode,focusOffset:a.focusOffset}),Mi&&Di(Mi,a)||(Mi=a,a=Br(qs,"onSelect"),0<a.length&&(t=new Ml("onSelect","select",null,t,n),e.push({event:t,listeners:a}),t.target=Ba)))}function sa(e,t){var n={};return n[e.toLowerCase()]=t.toLowerCase(),n["Webkit"+e]="webkit"+t,n["Moz"+e]="moz"+t,n}var Va={animationend:sa("Animation","AnimationEnd"),animationiteration:sa("Animation","AnimationIteration"),animationstart:sa("Animation","AnimationStart"),transitionrun:sa("Transition","TransitionRun"),transitionstart:sa("Transition","TransitionStart"),transitioncancel:sa("Transition","TransitionCancel"),transitionend:sa("Transition","TransitionEnd")},zs={},od={};vn&&(od=document.createElement("div").style,"AnimationEvent"in window||(delete Va.animationend.animation,delete Va.animationiteration.animation,delete Va.animationstart.animation),"TransitionEvent"in window||delete Va.transitionend.transition);function oa(e){if(zs[e])return zs[e];if(!Va[e])return e;var t=Va[e],n;for(n in t)if(t.hasOwnProperty(n)&&n in od)return zs[e]=t[n];return e}var cd=oa("animationend"),ud=oa("animationiteration"),dd=oa("animationstart"),y0=oa("transitionrun"),g0=oa("transitionstart"),x0=oa("transitioncancel"),_d=oa("transitionend"),md=new Map,Ds="abort auxClick beforeToggle cancel canPlay canPlayThrough click close contextMenu copy cut drag dragEnd dragEnter dragExit dragLeave dragOver dragStart drop durationChange emptied encrypted ended error fullscreenChange fullscreenError gotPointerCapture input invalid keyDown keyPress keyUp load loadedData loadedMetadata loadStart lostPointerCapture mouseDown mouseMove mouseOut mouseOver mouseUp paste pause play playing pointerCancel pointerDown pointerMove pointerOut pointerOver pointerUp progress rateChange reset resize seeked seeking stalled submit suspend timeUpdate touchCancel touchEnd touchStart volumeChange scroll toggle touchMove waiting wheel".split(" ");Ds.push("scrollEnd");function Zt(e,t){md.set(e,t),ra(t,[e])}var b0=0;function hn(e,t){if(e.name!=null&&e.name!=="auto")return e.name;if(t.autoName!==null)return t.autoName;e=Wt.identifierPrefix;var n=b0++;return e="_"+e+"t_"+n.toString(32)+"_",t.autoName=e}function fd(e){if(e==null||typeof e=="string")return e;var t=null,n=li;if(n!==null)for(var a=0;a<n.length;a++){var i=e[n[a]];if(i!=null){if(i==="none")return"none";t=t==null?i:t+(" "+i)}}return t??e.default}function yn(e,t){return e=fd(e),t=fd(t),t==null?e==="auto"?null:e:t==="auto"?null:t}var Vl=typeof reportError=="function"?reportError:function(e){if(typeof window=="object"&&typeof window.ErrorEvent=="function"){var t=new window.ErrorEvent("error",{bubbles:!0,cancelable:!0,message:typeof e=="object"&&e!==null&&typeof e.message=="string"?String(e.message):String(e),error:e});if(!window.dispatchEvent(t))return}else if(typeof process=="object"&&typeof process.emit=="function"){process.emit("uncaughtException",e);return}console.error(e)},Vt=[],La=0,Ms=0;function Ll(){for(var e=La,t=Ms=La=0;t<e;){var n=Vt[t];Vt[t++]=null;var a=Vt[t];Vt[t++]=null;var i=Vt[t];Vt[t++]=null;var l=Vt[t];if(Vt[t++]=null,a!==null&&i!==null){var s=a.pending;s===null?i.next=i:(i.next=s.next,s.next=i),a.pending=i}l!==0&&pd(n,i,l)}}function Hl(e,t,n,a){Vt[La++]=e,Vt[La++]=t,Vt[La++]=n,Vt[La++]=a,Ms|=a,e.lanes|=a,e=e.alternate,e!==null&&(e.lanes|=a)}function Us(e,t,n,a){return Hl(e,t,n,a),Gl(e)}function ca(e,t){return Hl(e,null,null,t),Gl(e)}function pd(e,t,n){e.lanes|=n;var a=e.alternate;a!==null&&(a.lanes|=n);for(var i=!1,l=e.return;l!==null;)l.childLanes|=n,a=l.alternate,a!==null&&(a.childLanes|=n),l.tag===22&&(e=l.stateNode,e===null||e._visibility&1||(i=!0)),e=l,l=l.return;return e.tag===3?(l=e.stateNode,i&&t!==null&&(i=31-Tt(n),e=l.hiddenUpdates,a=e[i],a===null?e[i]=[t]:a.push(t),t.lane=n|536870912),l):null}function Gl(e){if(50<il)throw il=0,Ar=null,Error(d(185));for(var t=e.return;t!==null;)e=t,t=e.return;return e.tag===3?e.stateNode:null}var Ha={};function w0(e,t,n,a){this.tag=e,this.key=n,this.sibling=this.child=this.return=this.stateNode=this.type=this.elementType=null,this.index=0,this.refCleanup=this.ref=null,this.pendingProps=t,this.dependencies=this.memoizedState=this.updateQueue=this.memoizedProps=null,this.mode=a,this.subtreeFlags=this.flags=0,this.deletions=null,this.childLanes=this.lanes=0,this.alternate=null}function bt(e,t,n,a){return new w0(e,t,n,a)}function Bs(e){return e=e.prototype,!(!e||!e.isReactComponent)}function gn(e,t){var n=e.alternate;return n===null?(n=bt(e.tag,t,e.key,e.mode),n.elementType=e.elementType,n.type=e.type,n.stateNode=e.stateNode,n.alternate=e,e.alternate=n):(n.pendingProps=t,n.type=e.type,n.flags=0,n.subtreeFlags=0,n.deletions=null),n.flags=e.flags&1206910976,n.childLanes=e.childLanes,n.lanes=e.lanes,n.child=e.child,n.memoizedProps=e.memoizedProps,n.memoizedState=e.memoizedState,n.updateQueue=e.updateQueue,t=e.dependencies,n.dependencies=t===null?null:{lanes:t.lanes,firstContext:t.firstContext},n.sibling=e.sibling,n.index=e.index,n.ref=e.ref,n.refCleanup=e.refCleanup,n}function vd(e,t){e.flags&=1206910978;var n=e.alternate;return n===null?(e.childLanes=0,e.lanes=t,e.child=null,e.subtreeFlags=0,e.memoizedProps=null,e.memoizedState=null,e.updateQueue=null,e.dependencies=null,e.stateNode=null):(e.childLanes=n.childLanes,e.lanes=n.lanes,e.child=n.child,e.subtreeFlags=0,e.deletions=null,e.memoizedProps=n.memoizedProps,e.memoizedState=n.memoizedState,e.updateQueue=n.updateQueue,e.type=n.type,t=n.dependencies,e.dependencies=t===null?null:{lanes:t.lanes,firstContext:t.firstContext}),e}function Yl(e,t,n,a,i,l){var s=0;if(a=e,typeof a=="function")Bs(a)&&(s=1);else if(typeof a=="string")s=Fv(e,n,Ge.current)?26:e==="html"||e==="head"||e==="body"?27:5;else e:switch(a){case We:return e=bt(31,n,t,i),e.elementType=We,e.lanes=l,e;case Me:return ua(n.children,i,l,t);case we:s=8,i|=24;break;case Je:return e=bt(12,n,t,i|2),e.elementType=Je,e.lanes=l,e;case te:return e=bt(13,n,t,i),e.elementType=te,e.lanes=l,e;case Q:return e=bt(19,n,t,i),e.elementType=Q,e.lanes=l,e;case Ut:case p:return e=i|32,e=bt(30,n,t,e),e.elementType=p,e.lanes=l,e.stateNode={autoName:null,paired:null,clones:null,ref:null},e;default:if(typeof a=="object"&&a!==null)switch(a.$$typeof){case Ve:s=10;break e;case it:s=9;break e;case D:s=11;break e;case ke:s=14;break e;case ve:s=16,a=null;break e}s=29,n=Error(d(130,e===null?"null":typeof e,"")),a=null}return t=bt(s,n,t,i),t.elementType=e,t.type=a,t.lanes=l,t}function ua(e,t,n,a){return e=bt(7,e,a,t),e.lanes=n,e}function Vs(e,t,n){return e=bt(6,e,null,t),e.lanes=n,e}function hd(e){var t=bt(18,null,null,0);return t.stateNode=e,t}function Ls(e,t,n){return t=bt(4,e.children!==null?e.children:[],e.key,t),t.lanes=n,t.stateNode={containerInfo:e.containerInfo,pendingChildren:null,implementation:e.implementation},t}var yd=new WeakMap;function Lt(e,t){if(typeof e=="object"&&e!==null){var n=yd.get(e);return n!==void 0?n:(t={value:e,source:t,stack:_u(t)},yd.set(e,t),t)}return{value:e,source:t,stack:_u(t)}}var Ga=[],Ya=0,Il=null,Ui=0,Ht=[],Gt=0,zn=null,nn=1,an="";function xn(e,t){Ga[Ya++]=Ui,Ga[Ya++]=Il,Il=e,Ui=t}function gd(e,t,n){Ht[Gt++]=nn,Ht[Gt++]=an,Ht[Gt++]=zn,zn=e;var a=nn;e=an;var i=32-Tt(a)-1;a&=~(1<<i),n+=1;var l=32-Tt(t)+i;if(30<l){var s=i-i%5;l=(a&(1<<s)-1).toString(32),a>>=s,i-=s,nn=1<<32-Tt(t)+i|n<<i|a,an=l+e}else nn=1<<l|n<<i|a,an=e}function Ql(e){e.return!==null&&(xn(e,1),gd(e,1,0))}function Hs(e){for(;e===Il;)Il=Ga[--Ya],Ga[Ya]=null,Ui=Ga[--Ya],Ga[Ya]=null;for(;e===zn;)zn=Ht[--Gt],Ht[Gt]=null,an=Ht[--Gt],Ht[Gt]=null,nn=Ht[--Gt],Ht[Gt]=null}function xd(e,t){Ht[Gt++]=nn,Ht[Gt++]=an,Ht[Gt++]=zn,nn=t.id,an=t.overflow,zn=e}var tt=null,Re=null,oe=!1,Dn=null,Yt=!1,Gs=Error(d(519));function Mn(e){var t=Error(d(418,1<arguments.length&&arguments[1]!==void 0&&arguments[1]?"text":"HTML",""));throw Bi(Lt(t,e)),Gs}function bd(e){var t=e.stateNode,n=e.type,a=e.memoizedProps;switch(t[lt]=e,t[xt]=a,n){case"dialog":de("cancel",t),de("close",t);break;case"iframe":case"object":case"embed":de("load",t);break;case"video":case"audio":for(n=0;n<rl.length;n++)de(rl[n],t);break;case"source":de("error",t);break;case"img":case"image":case"link":de("error",t),de("load",t);break;case"details":de("toggle",t);break;case"input":de("invalid",t),Ru(t,a.value,a.defaultValue,a.checked,a.defaultChecked,a.type,a.name,!0);break;case"select":de("invalid",t);break;case"textarea":de("invalid",t),Du(t,a.value,a.defaultValue,a.children)}n=a.children,typeof n!="string"&&typeof n!="number"&&typeof n!="bigint"||t.textContent===""+n||a.suppressHydrationWarning===!0||Gm(t.textContent,n)?(a.popover!=null&&(de("beforetoggle",t),de("toggle",t)),a.onScroll!=null&&de("scroll",t),a.onScrollEnd!=null&&de("scrollend",t),a.onClick!=null&&(t.onclick=tn),t=!0):t=!1,t||Mn(e,!0)}function Kl(e){for(tt=e.return;tt;)switch(tt.tag){case 5:case 31:case 13:Yt=!1;return;case 27:case 3:Yt=!0;return;default:tt=tt.return}}function Ia(e){if(e!==tt)return!1;if(!oe)return Kl(e),oe=!0,!1;var t=e.tag,n;if((n=t!==3&&t!==27)&&((n=t===5)&&(n=e.type,n=!(n!=="form"&&n!=="button")||gc(e.type,e.memoizedProps)),n=!n),n&&Re&&Mn(e),Kl(e),t===13){if(e=e.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(317));Re=of(e)}else if(t===31){if(e=e.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(317));Re=of(e)}else t===27?(t=Re,Wn(e.type)?(e=Tc,Tc=null,Re=e):Re=t):Re=tt?Qt(e.stateNode.nextSibling):null;return!0}function da(){Re=tt=null,oe=!1}function Ys(){var e=Dn;return e!==null&&(Nt===null?Nt=e:Nt.push.apply(Nt,e),Dn=null),e}function Bi(e){Dn===null?Dn=[e]:Dn.push(e)}var Is=Le(null),_a=null,bn=null;function Un(e,t,n){ce(Is,t._currentValue),t._currentValue=n}function wn(e){e._currentValue=Is.current,F(Is)}function Xl(e,t,n){for(;e!==null;){var a=e.alternate;if((e.childLanes&t)!==t?(e.childLanes|=t,a!==null&&(a.childLanes|=t)):a!==null&&(a.childLanes&t)!==t&&(a.childLanes|=t),e===n)break;e=e.return}}function Qs(e,t,n,a){var i=e.child;for(i!==null&&(i.return=e);i!==null;){var l=i.dependencies;if(l!==null){var s=i.child;l=l.firstContext;e:for(;l!==null;){var u=l;l=i;for(var _=0;_<t.length;_++)if(u.context===t[_]){l.lanes|=n,u=l.alternate,u!==null&&(u.lanes|=n),Xl(l.return,n,e),a||(s=null);break e}l=u.next}}else if(i.tag===18){if(s=i.return,s===null)throw Error(d(341));s.lanes|=n,l=s.alternate,l!==null&&(l.lanes|=n),Xl(s,n,e),s=null}else i.tag===13&&i.memoizedState!==null&&i.memoizedState.dehydrated===null?(i.lanes|=n,s=i.alternate,s!==null&&(s.lanes|=n),Xl(i.return,n,e),s=i.child,s=s!==null?s.sibling:null):s=i.child;if(s!==null)s.return=i;else for(s=i;s!==null;){if(s===e){s=null;break}if(i=s.sibling,i!==null){i.return=s.return,s=i;break}s=s.return}i=s}}function ma(e,t,n,a){e=null;for(var i=t,l=!1;i!==null;){if(!l){if((i.flags&524288)!==0)l=!0;else if((i.flags&262144)!==0)break}if(i.tag===10){var s=i.alternate;if(s===null)throw Error(d(387));if(s=s.memoizedProps,s!==null){var u=i.type;Ot(i.pendingProps.value,s.value)||(e!==null?e.push(u):e=[u])}}else if(i===en.current){if(s=i.alternate,s===null)throw Error(d(387));s.memoizedState.memoizedState!==i.memoizedState.memoizedState&&(e!==null?e.push(pi):e=[pi])}i=i.return}return e!==null&&Qs(t,e,n,a),t.flags|=262144,e!==null}function Zl(e){for(e=e.firstContext;e!==null;){if(!Ot(e.context._currentValue,e.memoizedValue))return!0;e=e.next}return!1}function fa(e){_a=e,bn=null,e=e.dependencies,e!==null&&(e.firstContext=null)}function rt(e){return wd(_a,e)}function Pl(e,t){return _a===null&&fa(e),wd(e,t)}function wd(e,t){var n=t._currentValue;if(t={context:t,memoizedValue:n,next:null},bn===null){if(e===null)throw Error(d(308));bn=t,e.dependencies={lanes:0,firstContext:t},e.flags|=524288}else bn=bn.next=t;return n}var k0=typeof AbortController<"u"?AbortController:function(){var e=[],t=this.signal={aborted:!1,addEventListener:function(n,a){e.push(a)}};this.abort=function(){t.aborted=!0,e.forEach(function(n){return n()})}},N0=o.unstable_scheduleCallback,S0=o.unstable_NormalPriority,Ke={$$typeof:Ve,Consumer:null,Provider:null,_currentValue:null,_currentValue2:null,_threadCount:0};function Ks(){return{controller:new k0,data:new Map,refCount:0}}function Vi(e){e.refCount--,e.refCount===0&&N0(S0,function(){e.controller.abort()})}function kd(e,t){if((e.pendingLanes&4194048)!==0){var n=e.transitionTypes;for(n===null&&(n=e.transitionTypes=[]),e=0;e<t.length;e++){var a=t[e];n.indexOf(a)===-1&&n.push(a)}}}var Li=null;function E0(e){var t=e.transitionTypes;return e.transitionTypes=null,t}var Hi=null,Xs=0,pa=0,Qa=null;function j0(e,t){if(Hi===null){var n=Hi=[];Xs=0,pa=uc(),Qa={status:"pending",value:void 0,then:function(a){n.push(a)}}}return Xs++,t.then(Nd,Nd),t}function Nd(){if(--Xs===0&&(Li=null,Hi!==null)){Qa!==null&&(Qa.status="fulfilled");var e=Hi;Hi=null,pa=0,Qa=null;for(var t=0;t<e.length;t++)(0,e[t])()}}function T0(e,t){var n=[],a={status:"pending",value:null,reason:null,then:function(i){n.push(i)}};return e.then(function(){a.status="fulfilled",a.value=t;for(var i=0;i<n.length;i++)(0,n[i])(t)},function(i){for(a.status="rejected",a.reason=i,i=0;i<n.length;i++)(0,n[i])(void 0)}),a}var Sd=R.S;R.S=function(e,t){if(hm=Et(),typeof t=="object"&&t!==null&&typeof t.then=="function"&&j0(e,t),Li!==null)for(var n=ci;n!==null;)kd(n,Li),n=n.next;if(n=e.types,n!==null){for(var a=ci;a!==null;)kd(a,n),a=a.next;if(pa!==0){a=Li,a===null&&(a=Li=[]);for(var i=0;i<n.length;i++){var l=n[i];a.indexOf(l)===-1&&a.push(l)}}}Sd!==null&&Sd(e,t)};var va=Le(null);function Zs(){var e=va.current;return e!==null?e:qe.pooledCache}function Fl(e,t){t===null?ce(va,va.current):ce(va,t.pool)}function Ed(){var e=Zs();return e===null?null:{parent:Ke._currentValue,pool:e}}var Ka=Error(d(460)),Ps=Error(d(474)),Jl=Error(d(542)),Wl={then:function(){}};function jd(e){return e=e.status,e==="fulfilled"||e==="rejected"}function Td(e,t,n){switch(n=e[n],n===void 0?e.push(t):n!==t&&(t.then(tn,tn),t=n),t.status){case"fulfilled":return t.value;case"rejected":throw e=t.reason,Od(e),e===void 0&&!("reason"in t)?Error(d(600)):e;default:if(typeof t.status=="string")t.then(tn,tn);else{if(e=qe,e!==null&&100<e.shellSuspendCounter)throw Error(d(482));e=t,e.status="pending",e.then(function(a){if(t.status==="pending"){var i=t;i.status="fulfilled",i.value=a}},function(a){if(t.status==="pending"){var i=t;i.status="rejected",i.reason=a}})}switch(t.status){case"fulfilled":return t.value;case"rejected":throw e=t.reason,Od(e),e}throw ya=t,Ka}}function ha(e){try{var t=e._init;return t(e._payload)}catch(n){throw n!==null&&typeof n=="object"&&typeof n.then=="function"?(ya=n,Ka):n}}var ya=null;function Cd(){if(ya===null)throw Error(d(459));var e=ya;return ya=null,e}function Od(e){if(e===Ka||e===Jl)throw Error(d(483))}var Xa=null,Gi=0;function $l(e){var t=Gi;return Gi+=1,Xa===null&&(Xa=[]),Td(Xa,e,t)}function Bn(e,t){t=t.props.ref,e.ref=t!==void 0?t:null}function er(e,t){throw t.$$typeof===ne?Error(d(525)):(e=Object.prototype.toString.call(t),Error(d(31,e==="[object Object]"?"object with keys {"+Object.keys(t).join(", ")+"}":e)))}function Ad(e){function t(h,f){if(e){var x=h.deletions;x===null?(h.deletions=[f],h.flags|=16):x.push(f)}}function n(h,f){if(!e)return null;for(;f!==null;)t(h,f),f=f.sibling;return null}function a(h){for(var f=new Map;h!==null;)h.key===null?f.set(h.index,h):f.set(h.key,h),h=h.sibling;return f}function i(h,f){return h=gn(h,f),h.index=0,h.sibling=null,h}function l(h,f,x){return h.index=x,e?(x=h.alternate,x!==null?(x=x.index,x<f?(h.flags|=2,f):x):(h.flags|=134217730,f)):(h.flags|=1048576,f)}function s(h){return e&&h.alternate===null&&(h.flags|=134217730),h}function u(h,f,x,E){return f===null||f.tag!==6?(f=Vs(x,h.mode,E),f.return=h,f):(f=i(f,x),f.return=h,f)}function _(h,f,x,E){var G=x.type;return G===Me?(h=w(h,f,x.props.children,E,x.key),Bn(h,x),h):f!==null&&(f.elementType===G||typeof G=="object"&&G!==null&&G.$$typeof===ve&&ha(G)===f.type)?(f=i(f,x.props),Bn(f,x),f.return=h,f):(f=Yl(x.type,x.key,x.props,null,h.mode,E),Bn(f,x),f.return=h,f)}function g(h,f,x,E){return f===null||f.tag!==4||f.stateNode.containerInfo!==x.containerInfo||f.stateNode.implementation!==x.implementation?(f=Ls(x,h.mode,E),f.return=h,f):(f=i(f,x.children||[]),f.return=h,f)}function w(h,f,x,E,G){return f===null||f.tag!==7?(f=ua(x,h.mode,E,G),f.return=h,f):(f=i(f,x),f.return=h,f)}function j(h,f,x){if(typeof f=="string"&&f!==""||typeof f=="number"||typeof f=="bigint")return f=Vs(""+f,h.mode,x),f.return=h,f;if(typeof f=="object"&&f!==null){switch(f.$$typeof){case De:return x=Yl(f.type,f.key,f.props,null,h.mode,x),Bn(x,f),x.return=h,x;case Be:return f=Ls(f,h.mode,x),f.return=h,f;case ve:return f=ha(f),j(h,f,x)}if(U(f)||$(f))return f=ua(f,h.mode,x,null),f.return=h,f;if(typeof f.then=="function")return j(h,$l(f),x);if(f.$$typeof===Ve)return j(h,Pl(h,f),x);er(h,f)}return null}function v(h,f,x,E){var G=f!==null?f.key:null;if(typeof x=="string"&&x!==""||typeof x=="number"||typeof x=="bigint")return G!==null?null:u(h,f,""+x,E);if(typeof x=="object"&&x!==null){switch(x.$$typeof){case De:return x.key===G?_(h,f,x,E):null;case Be:return x.key===G?g(h,f,x,E):null;case ve:return x=ha(x),v(h,f,x,E)}if(U(x)||$(x))return G!==null?null:w(h,f,x,E,null);if(typeof x.then=="function")return v(h,f,$l(x),E);if(x.$$typeof===Ve)return v(h,f,Pl(h,x),E);er(h,x)}return null}function b(h,f,x,E,G){if(typeof E=="string"&&E!==""||typeof E=="number"||typeof E=="bigint")return h=h.get(x)||null,u(f,h,""+E,G);if(typeof E=="object"&&E!==null){switch(E.$$typeof){case De:return h=h.get(E.key===null?x:E.key)||null,_(f,h,E,G);case Be:return h=h.get(E.key===null?x:E.key)||null,g(f,h,E,G);case ve:return E=ha(E),b(h,f,x,E,G)}if(U(E)||$(E))return h=h.get(x)||null,w(f,h,E,G,null);if(typeof E.then=="function")return b(h,f,x,$l(E),G);if(E.$$typeof===Ve)return b(h,f,x,Pl(f,E),G);er(f,E)}return null}function B(h,f,x,E){for(var G=null,pe=null,ee=f,ie=f=0,Pe=null;ee!==null&&ie<x.length;ie++){ee.index>ie?(Pe=ee,ee=null):Pe=ee.sibling;var xe=v(h,ee,x[ie],E);if(xe===null){ee===null&&(ee=Pe);break}e&&ee&&xe.alternate===null&&t(h,ee),f=l(xe,f,ie),pe===null?G=xe:pe.sibling=xe,pe=xe,ee=Pe}if(ie===x.length)return n(h,ee),oe&&xn(h,ie),G;if(ee===null){for(;ie<x.length;ie++)ee=j(h,x[ie],E),ee!==null&&(f=l(ee,f,ie),pe===null?G=ee:pe.sibling=ee,pe=ee);return oe&&xn(h,ie),G}for(ee=a(ee);ie<x.length;ie++)Pe=b(ee,h,ie,x[ie],E),Pe!==null&&(e&&(xe=Pe.alternate,xe!==null&&ee.delete(xe.key===null?ie:xe.key)),f=l(Pe,f,ie),pe===null?G=Pe:pe.sibling=Pe,pe=Pe);return e&&ee.forEach(function(aa){return t(h,aa)}),oe&&xn(h,ie),G}function X(h,f,x,E){if(x==null)throw Error(d(151));for(var G=null,pe=null,ee=f,ie=f=0,Pe=null,xe=x.next();ee!==null&&!xe.done;ie++,xe=x.next()){ee.index>ie?(Pe=ee,ee=null):Pe=ee.sibling;var aa=v(h,ee,xe.value,E);if(aa===null){ee===null&&(ee=Pe);break}e&&ee&&aa.alternate===null&&t(h,ee),f=l(aa,f,ie),pe===null?G=aa:pe.sibling=aa,pe=aa,ee=Pe}if(xe.done)return n(h,ee),oe&&xn(h,ie),G;if(ee===null){for(;!xe.done;ie++,xe=x.next())xe=j(h,xe.value,E),xe!==null&&(f=l(xe,f,ie),pe===null?G=xe:pe.sibling=xe,pe=xe);return oe&&xn(h,ie),G}for(ee=a(ee);!xe.done;ie++,xe=x.next())xe=b(ee,h,ie,xe.value,E),xe!==null&&(e&&(Pe=xe.alternate,Pe!==null&&ee.delete(Pe.key===null?ie:Pe.key)),f=l(xe,f,ie),pe===null?G=xe:pe.sibling=xe,pe=xe);return e&&ee.forEach(function(oh){return t(h,oh)}),oe&&xn(h,ie),G}function se(h,f,x,E){if(typeof x=="object"&&x!==null&&x.type===Me&&x.key===null&&x.props.ref===void 0&&(x=x.props.children),typeof x=="object"&&x!==null){switch(x.$$typeof){case De:e:{for(var G=x.key;f!==null;){if(f.key===G){if(G=x.type,G===Me){if(f.tag===7){n(h,f.sibling),E=i(f,x.props.children),Bn(E,x),E.return=h,h=E;break e}}else if(f.elementType===G||typeof G=="object"&&G!==null&&G.$$typeof===ve&&ha(G)===f.type){n(h,f.sibling),E=i(f,x.props),Bn(E,x),E.return=h,h=E;break e}n(h,f);break}else t(h,f);f=f.sibling}x.type===Me?(E=ua(x.props.children,h.mode,E,x.key),Bn(E,x),E.return=h,h=E):(E=Yl(x.type,x.key,x.props,null,h.mode,E),Bn(E,x),E.return=h,h=E)}return s(h);case Be:e:{for(G=x.key;f!==null;){if(f.key===G)if(f.tag===4&&f.stateNode.containerInfo===x.containerInfo&&f.stateNode.implementation===x.implementation){n(h,f.sibling),E=i(f,x.children||[]),E.return=h,h=E;break e}else{n(h,f);break}else t(h,f);f=f.sibling}E=Ls(x,h.mode,E),E.return=h,h=E}return s(h);case ve:return x=ha(x),se(h,f,x,E)}if(U(x))return B(h,f,x,E);if($(x)){if(G=$(x),typeof G!="function")throw Error(d(150));return x=G.call(x),X(h,f,x,E)}if(typeof x.then=="function")return se(h,f,$l(x),E);if(x.$$typeof===Ve)return se(h,f,Pl(h,x),E);er(h,x)}return typeof x=="string"&&x!==""||typeof x=="number"||typeof x=="bigint"?(x=""+x,f!==null&&f.tag===6?(n(h,f.sibling),E=i(f,x),E.return=h,h=E):(n(h,f),E=Vs(x,h.mode,E),E.return=h,h=E),s(h)):n(h,f)}return function(h,f,x,E){try{Gi=0;var G=se(h,f,x,E);return Xa=null,G}catch(ee){if(ee===Ka||ee===Jl)throw ee;var pe=bt(29,ee,null,h.mode);return pe.lanes=E,pe.return=h,pe}finally{}}}var ga=Ad(!0),qd=Ad(!1),Vn=!1;function Fs(e){e.updateQueue={baseState:e.memoizedState,firstBaseUpdate:null,lastBaseUpdate:null,shared:{pending:null,lanes:0,hiddenCallbacks:null},callbacks:null}}function Js(e,t){e=e.updateQueue,t.updateQueue===e&&(t.updateQueue={baseState:e.baseState,firstBaseUpdate:e.firstBaseUpdate,lastBaseUpdate:e.lastBaseUpdate,shared:e.shared,callbacks:null})}function Ln(e){return{lane:e,tag:0,payload:null,callback:null,next:null}}function Hn(e,t,n){var a=e.updateQueue;if(a===null)return null;if(a=a.shared,(Ee&2)!==0){var i=a.pending;return i===null?t.next=t:(t.next=i.next,i.next=t),a.pending=t,t=Gl(e),pd(e,null,n),t}return Hl(e,a,t,n),Gl(e)}function Yi(e,t,n){if(t=t.updateQueue,t!==null&&(t=t.shared,(n&4194048)!==0)){var a=t.lanes;a&=e.pendingLanes,n|=a,t.lanes=n,gu(e,n)}}function Ws(e,t){var n=e.updateQueue,a=e.alternate;if(a!==null&&(a=a.updateQueue,n===a)){var i=null,l=null;if(n=n.firstBaseUpdate,n!==null){do{var s={lane:n.lane,tag:n.tag,payload:n.payload,callback:null,next:null};l===null?i=l=s:l=l.next=s,n=n.next}while(n!==null);l===null?i=l=t:l=l.next=t}else i=l=t;n={baseState:a.baseState,firstBaseUpdate:i,lastBaseUpdate:l,shared:a.shared,callbacks:a.callbacks},e.updateQueue=n;return}e=n.lastBaseUpdate,e===null?n.firstBaseUpdate=t:e.next=t,n.lastBaseUpdate=t}var $s=!1;function Ii(){if($s){var e=Qa;if(e!==null)throw e}}function Qi(e,t,n,a){$s=!1;var i=e.updateQueue;Vn=!1;var l=i.firstBaseUpdate,s=i.lastBaseUpdate,u=i.shared.pending;if(u!==null){i.shared.pending=null;var _=u,g=_.next;_.next=null,s===null?l=g:s.next=g,s=_;var w=e.alternate;w!==null&&(w=w.updateQueue,u=w.lastBaseUpdate,u!==s&&(u===null?w.firstBaseUpdate=g:u.next=g,w.lastBaseUpdate=_))}if(l!==null){var j=i.baseState;s=0,w=g=_=null,u=l;do{var v=u.lane&-536870913,b=v!==u.lane;if(b?(fe&v)===v:(a&v)===v){v!==0&&v===pa&&($s=!0),w!==null&&(w=w.next={lane:0,tag:u.tag,payload:u.payload,callback:null,next:null});e:{var B=e,X=u;v=t;var se=n;switch(X.tag){case 1:if(B=X.payload,typeof B=="function"){j=B.call(se,j,v);break e}j=B;break e;case 3:B.flags=B.flags&-65537|128;case 0:if(B=X.payload,v=typeof B=="function"?B.call(se,j,v):B,v==null)break e;j=H({},j,v);break e;case 2:Vn=!0}}v=u.callback,v!==null&&(e.flags|=64,b&&(e.flags|=8192),b=i.callbacks,b===null?i.callbacks=[v]:b.push(v))}else b={lane:v,tag:u.tag,payload:u.payload,callback:u.callback,next:null},w===null?(g=w=b,_=j):w=w.next=b,s|=v;if(u=u.next,u===null){if(u=i.shared.pending,u===null)break;b=u,u=b.next,b.next=null,i.lastBaseUpdate=b,i.shared.pending=null}}while(!0);w===null&&(_=j),i.baseState=_,i.firstBaseUpdate=g,i.lastBaseUpdate=w,l===null&&(i.shared.lanes=0),Zn|=s,e.lanes=s,e.memoizedState=j}}function Rd(e,t){if(typeof e!="function")throw Error(d(191,e));e.call(t)}function zd(e,t){var n=e.callbacks;if(n!==null)for(e.callbacks=null,e=0;e<n.length;e++)Rd(n[e],t)}var Gn=Le(null),tr=Le(0);function Dd(e,t){e=jn,ce(tr,e),ce(Gn,t),jn=e|t.baseLanes}function eo(){ce(tr,jn),ce(Gn,Gn.current)}function to(){jn=tr.current,F(Gn),F(tr)}var st=Le(null),mt=null;function Yn(e){var t=e.alternate;ce(ot,ot.current&1),ce(st,e),mt===null&&(t===null||Gn.current!==null||t.memoizedState!==null)&&(mt=e)}function no(e){ce(ot,ot.current),ce(st,e),mt===null&&(mt=e)}function Md(e){e.tag===22?(ce(ot,ot.current),ce(st,e),mt===null&&(mt=e)):In()}function In(){ce(ot,ot.current),ce(st,st.current)}function At(e){F(st),mt===e&&(mt=null),F(ot)}var ot=Le(0);function Ki(e,t){ce(st,st.current),ce(ot,t)}function ao(e){F(ot),F(st),mt===e&&(mt=null)}function nr(e){for(var t=e;t!==null;){if(t.tag===13){var n=t.memoizedState;if(n!==null&&(n=n.dehydrated,n===null||Ec(n)||jc(n)))return t}else if(t.tag===19&&t.memoizedProps.revealOrder!=="independent"){if((t.flags&128)!==0)return t}else if(t.child!==null){t.child.return=t,t=t.child;continue}if(t===e)break;for(;t.sibling===null;){if(t.return===null||t.return===e)return null;t=t.return}t.sibling.return=t.return,t=t.sibling}return null}var kn=0,re=null,Ae=null,Xe=null,ar=!1,Za=!1,xa=!1,ir=0,Xi=0,Pa=null,C0=0;function Ye(){throw Error(d(321))}function io(e,t){if(t===null)return!1;for(var n=0;n<t.length&&n<e.length;n++)if(!Ot(e[n],t[n]))return!1;return!0}function lo(e,t,n,a,i,l){return kn=l,re=t,t.memoizedState=null,t.updateQueue=null,t.lanes=0,R.H=e===null||e.memoizedState===null?g_:x_,xa=!1,l=n(a,i),xa=!1,Za&&(l=Bd(t,n,a,i)),Ud(e),l}function Ud(e){R.H=dr;var t=Ae!==null&&Ae.next!==null;if(kn=0,Xe=Ae=re=null,ar=!1,Xi=0,Pa=null,t)throw Error(d(300));e===null||Ze||(e=e.dependencies,e!==null&&Zl(e)&&(Ze=!0))}function Bd(e,t,n,a){re=e;var i=0;do{if(Za&&(Pa=null),Xi=0,Za=!1,25<=i)throw Error(d(301));if(i+=1,Xe=Ae=null,e.updateQueue!=null){var l=e.updateQueue;l.lastEffect=null,l.events=null,l.stores=null,l.memoCache!=null&&(l.memoCache.index=0)}R.H=U0,l=t(n,a)}while(Za);return l}function O0(){var e=R.H,t=e.useState()[0];return t=typeof t.then=="function"?Zi(t):t,e=e.useState()[0],(Ae!==null?Ae.memoizedState:null)!==e&&(re.flags|=1024),t}function ro(){var e=ir!==0;return ir=0,e}function so(e,t,n){t.updateQueue=e.updateQueue,t.flags&=-2053,e.lanes&=~n}function oo(e){if(ar){for(e=e.memoizedState;e!==null;){var t=e.queue;t!==null&&(t.pending=null),e=e.next}ar=!1}kn=0,Xe=Ae=re=null,Za=!1,Xi=ir=0,Pa=null}function pt(){var e={memoizedState:null,baseState:null,baseQueue:null,queue:null,next:null};return Xe===null?re.memoizedState=Xe=e:Xe=Xe.next=e,Xe}function Qe(){if(Ae===null){var e=re.alternate;e=e!==null?e.memoizedState:null}else e=Ae.next;var t=Xe===null?re.memoizedState:Xe.next;if(t!==null)Xe=t,Ae=e;else{if(e===null)throw re.alternate===null?Error(d(467)):Error(d(310));Ae=e,e={memoizedState:Ae.memoizedState,baseState:Ae.baseState,baseQueue:Ae.baseQueue,queue:Ae.queue,next:null},Xe===null?re.memoizedState=Xe=e:Xe=Xe.next=e}return Xe}function lr(){return{lastEffect:null,events:null,stores:null,memoCache:null}}function Zi(e){var t=Xi;return Xi+=1,Pa===null&&(Pa=[]),e=Td(Pa,e,t),t=re,(Xe===null?t.memoizedState:Xe.next)===null&&(t=t.alternate,R.H=t===null||t.memoizedState===null?g_:x_),e}function rr(e){if(e!==null&&typeof e=="object"){if(typeof e.then=="function")return Zi(e);if(e.$$typeof===A)return;if(e.$$typeof===Ve)return rt(e)}throw Error(d(438,String(e)))}function co(e){var t=null,n=re.updateQueue;if(n!==null&&(t=n.memoCache),t==null){var a=re.alternate;a!==null&&(a=a.updateQueue,a!==null&&(a=a.memoCache,a!=null&&(t={data:a.data.map(function(i){return i.slice()}),index:0})))}if(t==null&&(t={data:[],index:0}),n===null&&(n=lr(),re.updateQueue=n),n.memoCache=t,n=t.data[t.index],n===void 0)for(n=t.data[t.index]=Array(e),a=0;a<e;a++)n[a]=mn;return t.index++,n}function Nn(e,t){return typeof t=="function"?t(e):t}function sr(e){var t=Qe();return uo(t,Ae,e)}function uo(e,t,n){var a=e.queue;if(a===null)throw Error(d(311));a.lastRenderedReducer=n;var i=e.baseQueue,l=a.pending;if(l!==null){if(i!==null){var s=i.next;i.next=l.next,l.next=s}t.baseQueue=i=l,a.pending=null}if(l=e.baseState,i===null)e.memoizedState=l;else{t=i.next;var u=s=null,_=null,g=t,w=!1;do{var j=g.lane&-536870913;if(j!==g.lane?(fe&j)===j:(kn&j)===j){var v=g.revertLane;if(v===0)_!==null&&(_=_.next={lane:0,revertLane:0,gesture:null,action:g.action,hasEagerState:g.hasEagerState,eagerState:g.eagerState,next:null}),j===pa&&(w=!0);else if((kn&v)===v){g=g.next,v===pa&&(w=!0);continue}else j={lane:0,revertLane:g.revertLane,gesture:null,action:g.action,hasEagerState:g.hasEagerState,eagerState:g.eagerState,next:null},_===null?(u=_=j,s=l):_=_.next=j,re.lanes|=v,Zn|=v;j=g.action,xa&&n(l,j),l=g.hasEagerState?g.eagerState:n(l,j)}else v={lane:j,revertLane:g.revertLane,gesture:g.gesture,action:g.action,hasEagerState:g.hasEagerState,eagerState:g.eagerState,next:null},_===null?(u=_=v,s=l):_=_.next=v,re.lanes|=j,Zn|=j;g=g.next}while(g!==null&&g!==t);if(_===null?s=l:_.next=u,!Ot(l,e.memoizedState)&&(Ze=!0,w&&(n=Qa,n!==null)))throw n;e.memoizedState=l,e.baseState=s,e.baseQueue=_,a.lastRenderedState=l}return i===null&&(a.lanes=0),[e.memoizedState,a.dispatch]}function _o(e){var t=Qe(),n=t.queue;if(n===null)throw Error(d(311));n.lastRenderedReducer=e;var a=n.dispatch,i=n.pending,l=t.memoizedState;if(i!==null){n.pending=null;var s=i=i.next;do l=e(l,s.action),s=s.next;while(s!==i);Ot(l,t.memoizedState)||(Ze=!0),t.memoizedState=l,t.baseQueue===null&&(t.baseState=l),n.lastRenderedState=l}return[l,a]}function Vd(e,t,n){var a=re,i=Qe(),l=oe;if(l){if(n===void 0)throw Error(d(407));n=n()}else n=t();var s=!Ot((Ae||i).memoizedState,n);if(s&&(i.memoizedState=n,Ze=!0),i=i.queue,po(Gd.bind(null,a,i,e),[e]),e=i.getSnapshot!==t||s||Xe!==null&&(Xe.memoizedState.tag&1)!==0,Fa(e?9:8,{destroy:void 0},Hd.bind(null,a,i,n,t),null),e){if(a.flags|=2048,qe===null)throw Error(d(349));l||(kn&127)!==0||Ld(a,t,n)}return n}function Ld(e,t,n){e.flags|=16384,e={getSnapshot:t,value:n},t=re.updateQueue,t===null?(t=lr(),re.updateQueue=t,t.stores=[e]):(n=t.stores,n===null?t.stores=[e]:n.push(e))}function Hd(e,t,n,a){t.value=n,t.getSnapshot=a,Yd(t)&&Id(e)}function Gd(e,t,n){return n(function(){Yd(t)&&Id(e)})}function Yd(e){var t=e.getSnapshot;e=e.value;try{var n=t();return!Ot(e,n)}catch{return!0}}function Id(e){var t=ca(e,2);t!==null&&St(t,e,2)}function mo(e){var t=pt();if(typeof e=="function"){var n=e;if(e=n(),xa){An(!0);try{n()}finally{An(!1)}}}return t.memoizedState=t.baseState=e,t.queue={pending:null,lanes:0,dispatch:null,lastRenderedReducer:Nn,lastRenderedState:e},t}function Qd(e,t,n,a){return e.baseState=n,uo(e,Ae,typeof a=="function"?a:Nn)}function A0(e,t,n,a,i){if(ur(e))throw Error(d(485));if(e=t.action,e!==null){var l={payload:i,action:e,next:null,isTransition:!0,status:"pending",value:null,reason:null,listeners:[],then:function(s){l.listeners.push(s)}};R.T!==null?n(!0):l.isTransition=!1,a(l),n=t.pending,n===null?(l.next=t.pending=l,Kd(t,l)):(l.next=n.next,t.pending=n.next=l)}}function Kd(e,t){var n=t.action,a=t.payload,i=e.state;if(t.isTransition){var l=R.T,s={};s.types=l!==null?l.types:null,R.T=s;try{var u=n(i,a),_=R.S;_!==null&&_(s,u),Xd(e,t,u)}catch(g){fo(e,t,g)}finally{l!==null&&s.types!==null&&(l.types=s.types),R.T=l}}else try{l=n(i,a),Xd(e,t,l)}catch(g){fo(e,t,g)}}function Xd(e,t,n){n!==null&&typeof n=="object"&&typeof n.then=="function"?n.then(function(a){Zd(e,t,a)},function(a){return fo(e,t,a)}):Zd(e,t,n)}function Zd(e,t,n){t.status="fulfilled",t.value=n,Pd(t),e.state=n,t=e.pending,t!==null&&(n=t.next,n===t?e.pending=null:(n=n.next,t.next=n,Kd(e,n)))}function fo(e,t,n){var a=e.pending;if(e.pending=null,a!==null){a=a.next;do t.status="rejected",t.reason=n,Pd(t),t=t.next;while(t!==a)}e.action=null}function Pd(e){e=e.listeners;for(var t=0;t<e.length;t++)(0,e[t])()}function Fd(e,t){return t}function Jd(e,t){if(oe){var n=qe.formState;if(n!==null){e:{var a=re;if(oe){if(Re){t:{for(var i=Re,l=Yt;i.nodeType!==8;){if(!l){i=null;break t}if(i=Qt(i.nextSibling),i===null){i=null;break t}}l=i.data,i=l==="F!"||l==="F"?i:null}if(i){Re=Qt(i.nextSibling),a=i.data==="F!";break e}}Mn(a)}a=!1}a&&(t=n[0])}}return n=pt(),n.memoizedState=n.baseState=t,a={pending:null,lanes:0,dispatch:null,lastRenderedReducer:Fd,lastRenderedState:t},n.queue=a,n=v_.bind(null,re,a),a.dispatch=n,a=mo(!1),l=xo.bind(null,re,!1,a.queue),a=pt(),i={state:t,dispatch:null,action:e,pending:null},a.queue=i,n=A0.bind(null,re,i,l,n),i.dispatch=n,a.memoizedState=e,[t,n,!1]}function Wd(e){var t=Qe();return $d(t,Ae,e)}function $d(e,t,n){if(t=uo(e,t,Fd)[0],e=sr(Nn)[0],typeof t=="object"&&t!==null&&typeof t.then=="function")try{var a=Zi(t)}catch(s){throw s===Ka?Jl:s}else a=t;t=Qe();var i=t.queue,l=i.dispatch;return n!==t.memoizedState&&(re.flags|=2048,Fa(9,{destroy:void 0},q0.bind(null,i,n),null)),[a,l,e]}function q0(e,t){e.action=t}function e_(e){var t=Qe(),n=Ae;if(n!==null)return $d(t,n,e);Qe(),t=t.memoizedState,n=Qe();var a=n.queue.dispatch;return n.memoizedState=e,[t,a,!1]}function Fa(e,t,n,a){return e={tag:e,create:n,deps:a,inst:t,next:null},t=re.updateQueue,t===null&&(t=lr(),re.updateQueue=t),n=t.lastEffect,n===null?t.lastEffect=e.next=e:(a=n.next,n.next=e,e.next=a,t.lastEffect=e),e}function t_(){return Qe().memoizedState}function or(e,t,n,a){var i=pt();re.flags|=e,i.memoizedState=Fa(1|t,{destroy:void 0},n,a===void 0?null:a)}function cr(e,t,n,a){var i=Qe();a=a===void 0?null:a;var l=i.memoizedState.inst;Ae!==null&&a!==null&&io(a,Ae.memoizedState.deps)?i.memoizedState=Fa(t,l,n,a):(re.flags|=e,i.memoizedState=Fa(1|t,l,n,a))}function n_(e,t){or(8390656,8,e,t)}function po(e,t){cr(2048,8,e,t)}function R0(e){re.flags|=4;var t=re.updateQueue;if(t===null)t=lr(),re.updateQueue=t,t.events=[e];else{var n=t.events;n===null?t.events=[e]:n.push(e)}}function a_(e){var t=Qe().memoizedState;return R0({ref:t,nextImpl:e}),function(){if((Ee&2)!==0)throw Error(d(440));return t.impl.apply(void 0,arguments)}}function i_(e,t){return cr(4,2,e,t)}function l_(e,t){return cr(4,4,e,t)}function r_(e,t){if(typeof t=="function"){e=e();var n=t(e);return function(){typeof n=="function"?n():t(null)}}if(t!=null)return e=e(),t.current=e,function(){t.current=null}}function s_(e,t,n){n=n!=null?n.concat([e]):null,cr(4,4,r_.bind(null,t,e),n)}function vo(){}function o_(e,t){var n=Qe();t=t===void 0?null:t;var a=n.memoizedState;return t!==null&&io(t,a[1])?a[0]:(n.memoizedState=[e,t],e)}function c_(e,t){var n=Qe();t=t===void 0?null:t;var a=n.memoizedState;if(t!==null&&io(t,a[1]))return a[0];if(a=e(),xa){An(!0);try{e()}finally{An(!1)}}return n.memoizedState=[a,t],a}function ho(e,t,n){return n===void 0||(kn&1073741824)!==0&&(fe&261930)===0?e.memoizedState=t:(e.memoizedState=n,e=gm(),re.lanes|=e,Zn|=e,n)}function u_(e,t,n,a){return Ot(n,t)?n:Gn.current!==null?(e=ho(e,n,a),Ot(e,t)||(Ze=!0),e):(kn&106)===0||(kn&1073741824)!==0&&(fe&261930)===0?(Ze=!0,e.memoizedState=n):(e=gm(),re.lanes|=e,Zn|=e,t)}function d_(e,t,n,a,i){var l=N.p;N.p=l!==0&&8>l?l:8;var s=R.T,u={};u.types=s!==null?s.types:null,R.T=u,xo(e,!1,t,n);try{var _=i(),g=R.S;if(g!==null&&g(u,_),_!==null&&typeof _=="object"&&typeof _.then=="function"){var w=T0(_,a);Pi(e,t,w,Dt(e))}else Pi(e,t,a,Dt(e))}catch(j){Pi(e,t,{then:function(){},status:"rejected",reason:j},Dt())}finally{N.p=l,s!==null&&u.types!==null&&(s.types=u.types),R.T=s}}function z0(){}function yo(e,t,n,a){if(e.tag!==5)throw Error(d(476));var i=__(e).queue;d_(e,i,t,ge,n===null?z0:function(){return m_(e),n(a)})}function __(e){var t=e.memoizedState;if(t!==null)return t;t={memoizedState:ge,baseState:ge,baseQueue:null,queue:{pending:null,lanes:0,dispatch:null,lastRenderedReducer:Nn,lastRenderedState:ge},next:null};var n={};return t.next={memoizedState:n,baseState:n,baseQueue:null,queue:{pending:null,lanes:0,dispatch:null,lastRenderedReducer:Nn,lastRenderedState:n},next:null},e.memoizedState=t,e=e.alternate,e!==null&&(e.memoizedState=t),t}function m_(e){var t=__(e);t.next===null&&(t=e.alternate.memoizedState),Pi(e,t.next.queue,{},Dt())}function go(){return rt(pi)}function f_(){return Qe().memoizedState}function p_(){return Qe().memoizedState}function D0(e){for(var t=e.return;t!==null;){switch(t.tag){case 24:case 3:var n=Dt();e=Ln(n);var a=Hn(t,e,n);a!==null&&(St(a,t,n),Yi(a,t,n)),t={cache:Ks()},e.payload=t;return}t=t.return}}function M0(e,t,n){var a=Dt();n={lane:a,revertLane:0,gesture:null,action:n,hasEagerState:!1,eagerState:null,next:null},ur(e)?h_(t,n):(n=Us(e,t,n,a),n!==null&&(St(n,e,a),y_(n,t,a)))}function v_(e,t,n){var a=Dt();Pi(e,t,n,a)}function Pi(e,t,n,a){var i={lane:a,revertLane:0,gesture:null,action:n,hasEagerState:!1,eagerState:null,next:null};if(ur(e))h_(t,i);else{var l=e.alternate;if(e.lanes===0&&(l===null||l.lanes===0)&&(l=t.lastRenderedReducer,l!==null))try{var s=t.lastRenderedState,u=l(s,n);if(i.hasEagerState=!0,i.eagerState=u,Ot(u,s))return Hl(e,t,i,0),qe===null&&Ll(),!1}catch{}finally{}if(n=Us(e,t,i,a),n!==null)return St(n,e,a),y_(n,t,a),!0}return!1}function xo(e,t,n,a){if(a={lane:2,revertLane:uc(),gesture:null,action:a,hasEagerState:!1,eagerState:null,next:null},ur(e)){if(t)throw Error(d(479))}else t=Us(e,n,a,2),t!==null&&St(t,e,2)}function ur(e){var t=e.alternate;return e===re||t!==null&&t===re}function h_(e,t){Za=ar=!0;var n=e.pending;n===null?t.next=t:(t.next=n.next,n.next=t),e.pending=t}function y_(e,t,n){if((n&4194048)!==0){var a=t.lanes;a&=e.pendingLanes,n|=a,t.lanes=n,gu(e,n)}}var dr={readContext:rt,use:rr,useCallback:Ye,useContext:Ye,useEffect:Ye,useImperativeHandle:Ye,useLayoutEffect:Ye,useInsertionEffect:Ye,useMemo:Ye,useReducer:Ye,useRef:Ye,useState:Ye,useDebugValue:Ye,useDeferredValue:Ye,useTransition:Ye,useSyncExternalStore:Ye,useId:Ye,useHostTransitionStatus:Ye,useFormState:Ye,useActionState:Ye,useOptimistic:Ye,useMemoCache:Ye,useCacheRefresh:Ye,useEffectEvent:Ye},g_={readContext:rt,use:rr,useCallback:function(e,t){return pt().memoizedState=[e,t===void 0?null:t],e},useContext:rt,useEffect:n_,useImperativeHandle:function(e,t,n){n=n!=null?n.concat([e]):null,or(4194308,4,r_.bind(null,t,e),n)},useLayoutEffect:function(e,t){return or(4194308,4,e,t)},useInsertionEffect:function(e,t){or(4,2,e,t)},useMemo:function(e,t){var n=pt();t=t===void 0?null:t;var a=e();if(xa){An(!0);try{e()}finally{An(!1)}}return n.memoizedState=[a,t],a},useReducer:function(e,t,n){var a=pt();if(n!==void 0){var i=n(t);if(xa){An(!0);try{n(t)}finally{An(!1)}}}else i=t;return a.memoizedState=a.baseState=i,e={pending:null,lanes:0,dispatch:null,lastRenderedReducer:e,lastRenderedState:i},a.queue=e,e=e.dispatch=M0.bind(null,re,e),[a.memoizedState,e]},useRef:function(e){var t=pt();return e={current:e},t.memoizedState=e},useState:function(e){e=mo(e);var t=e.queue,n=v_.bind(null,re,t);return t.dispatch=n,[e.memoizedState,n]},useDebugValue:vo,useDeferredValue:function(e,t){var n=pt();return ho(n,e,t)},useTransition:function(){var e=mo(!1);return e=d_.bind(null,re,e.queue,!0,!1),pt().memoizedState=e,[!1,e]},useSyncExternalStore:function(e,t,n){var a=re,i=pt();if(oe){if(n===void 0)throw Error(d(407));n=n()}else{if(n=t(),qe===null)throw Error(d(349));(fe&127)!==0||Ld(a,t,n)}i.memoizedState=n;var l={value:n,getSnapshot:t};return i.queue=l,n_(Gd.bind(null,a,l,e),[e]),a.flags|=2048,Fa(9,{destroy:void 0},Hd.bind(null,a,l,n,t),null),n},useId:function(){var e=pt(),t=qe.identifierPrefix;if(oe){var n=an,a=nn;n=(a&~(1<<32-Tt(a)-1)).toString(32)+n,t="_"+t+"R_"+n,n=ir++,0<n&&(t+="H"+n.toString(32)),t+="_"}else n=C0++,t="_"+t+"r_"+n.toString(32)+"_";return e.memoizedState=t},useHostTransitionStatus:go,useFormState:Jd,useActionState:Jd,useOptimistic:function(e){var t=pt();t.memoizedState=t.baseState=e;var n={pending:null,lanes:0,dispatch:null,lastRenderedReducer:null,lastRenderedState:null};return t.queue=n,t=xo.bind(null,re,!0,n),n.dispatch=t,[e,t]},useMemoCache:co,useCacheRefresh:function(){return pt().memoizedState=D0.bind(null,re)},useEffectEvent:function(e){var t=pt(),n={impl:e};return t.memoizedState=n,function(){if((Ee&2)!==0)throw Error(d(440));return n.impl.apply(void 0,arguments)}}},x_={readContext:rt,use:rr,useCallback:o_,useContext:rt,useEffect:po,useImperativeHandle:s_,useInsertionEffect:i_,useLayoutEffect:l_,useMemo:c_,useReducer:sr,useRef:t_,useState:function(){return sr(Nn)},useDebugValue:vo,useDeferredValue:function(e,t){var n=Qe();return u_(n,Ae.memoizedState,e,t)},useTransition:function(){var e=sr(Nn)[0],t=Qe().memoizedState;return[typeof e=="boolean"?e:Zi(e),t]},useSyncExternalStore:Vd,useId:f_,useHostTransitionStatus:go,useFormState:Wd,useActionState:Wd,useOptimistic:function(e,t){var n=Qe();return Qd(n,Ae,e,t)},useMemoCache:co,useCacheRefresh:p_,useEffectEvent:a_},U0={readContext:rt,use:rr,useCallback:o_,useContext:rt,useEffect:po,useImperativeHandle:s_,useInsertionEffect:i_,useLayoutEffect:l_,useMemo:c_,useReducer:_o,useRef:t_,useState:function(){return _o(Nn)},useDebugValue:vo,useDeferredValue:function(e,t){var n=Qe();return Ae===null?ho(n,e,t):u_(n,Ae.memoizedState,e,t)},useTransition:function(){var e=_o(Nn)[0],t=Qe().memoizedState;return[typeof e=="boolean"?e:Zi(e),t]},useSyncExternalStore:Vd,useId:f_,useHostTransitionStatus:go,useFormState:e_,useActionState:e_,useOptimistic:function(e,t){var n=Qe();return Ae!==null?Qd(n,Ae,e,t):(n.baseState=e,[e,n.queue.dispatch])},useMemoCache:co,useCacheRefresh:p_,useEffectEvent:a_};function bo(e,t,n,a){t=e.memoizedState,n=n(a,t),n=n==null?t:H({},t,n),e.memoizedState=n,e.lanes===0&&(e.updateQueue.baseState=n)}var wo={enqueueSetState:function(e,t,n){e=e._reactInternals;var a=Dt(),i=Ln(a);i.payload=t,n!=null&&(i.callback=n),t=Hn(e,i,a),t!==null&&(St(t,e,a),Yi(t,e,a))},enqueueReplaceState:function(e,t,n){e=e._reactInternals;var a=Dt(),i=Ln(a);i.tag=1,i.payload=t,n!=null&&(i.callback=n),t=Hn(e,i,a),t!==null&&(St(t,e,a),Yi(t,e,a))},enqueueForceUpdate:function(e,t){e=e._reactInternals;var n=Dt(),a=Ln(n);a.tag=2,t!=null&&(a.callback=t),t=Hn(e,a,n),t!==null&&(St(t,e,n),Yi(t,e,n))}};function b_(e,t,n,a,i,l,s){return e=e.stateNode,typeof e.shouldComponentUpdate=="function"?e.shouldComponentUpdate(a,l,s):t.prototype&&t.prototype.isPureReactComponent?!Di(n,a)||!Di(i,l):!0}function w_(e,t,n,a){e=t.state,typeof t.componentWillReceiveProps=="function"&&t.componentWillReceiveProps(n,a),typeof t.UNSAFE_componentWillReceiveProps=="function"&&t.UNSAFE_componentWillReceiveProps(n,a),t.state!==e&&wo.enqueueReplaceState(t,t.state,null)}function ba(e,t){var n=t;if("ref"in t){n={};for(var a in t)a!=="ref"&&(n[a]=t[a])}if(e=e.defaultProps){n===t&&(n=H({},n));for(var i in e)n[i]===void 0&&(n[i]=e[i])}return n}function k_(e){Vl(e)}function N_(e){console.error(e)}function S_(e){Vl(e)}function _r(e,t){try{var n=e.onUncaughtError;n(t.value,{componentStack:t.stack})}catch(a){setTimeout(function(){throw a})}}function E_(e,t,n){try{var a=e.onCaughtError;a(n.value,{componentStack:n.stack,errorBoundary:t.tag===1?t.stateNode:null})}catch(i){setTimeout(function(){throw i})}}function ko(e,t,n){return n=Ln(n),n.tag=3,n.payload={element:null},n.callback=function(){_r(e,t)},n}function j_(e){return e=Ln(e),e.tag=3,e}function T_(e,t,n,a){var i=n.type.getDerivedStateFromError;if(typeof i=="function"){var l=a.value;e.payload=function(){return i(l)},e.callback=function(){E_(t,n,a)}}var s=n.stateNode;s!==null&&typeof s.componentDidCatch=="function"&&(e.callback=function(){E_(t,n,a),typeof i!="function"&&(Pn===null?Pn=new Set([this]):Pn.add(this));var u=a.stack;this.componentDidCatch(a.value,{componentStack:u!==null?u:""})})}function B0(e,t,n,a,i){if(n.flags|=32768,a!==null&&typeof a=="object"&&typeof a.then=="function"){if(t=n.alternate,t!==null&&ma(t,n,i,!0),n=st.current,n!==null){switch(n.tag){case 31:case 13:case 19:return mt===null?Rr():n.alternate===null&&Ie===0&&(Ie=3),n.flags&=-257,n.flags|=65536,n.lanes=i,a===Wl?n.flags|=16384:(t=n.updateQueue,t===null?n.updateQueue=new Set([a]):t.add(a),sc(e,a,i)),!1;case 22:return n.flags|=65536,a===Wl?n.flags|=16384:(t=n.updateQueue,t===null?(t={transitions:null,markerInstances:null,retryQueue:new Set([a])},n.updateQueue=t):(n=t.retryQueue,n===null?t.retryQueue=new Set([a]):n.add(a)),sc(e,a,i)),!1}throw Error(d(435,n.tag))}return sc(e,a,i),Rr(),!1}if(oe)return t=st.current,t!==null?((t.flags&65536)===0&&(t.flags|=256),t.flags|=65536,t.lanes=i,a!==Gs&&(e=Error(d(422),{cause:a}),Bi(Lt(e,n)))):(a!==Gs&&(t=Error(d(423),{cause:a}),Bi(Lt(t,n))),e=e.current.alternate,e.flags|=65536,i&=-i,e.lanes|=i,a=Lt(a,n),i=ko(e.stateNode,a,i),Ws(e,i),Ie!==4&&(Ie=2)),!1;var l=Error(d(520),{cause:a});if(l=Lt(l,n),al===null?al=[l]:al.push(l),Ie!==4&&(Ie=2),t===null)return!0;a=Lt(a,n),n=t;do{switch(n.tag){case 3:return n.flags|=65536,e=i&-i,n.lanes|=e,e=ko(n.stateNode,a,e),Ws(n,e),!1;case 1:if(t=n.type,l=n.stateNode,(n.flags&128)===0&&(typeof t.getDerivedStateFromError=="function"||l!==null&&typeof l.componentDidCatch=="function"&&(Pn===null||!Pn.has(l))))return n.flags|=65536,i&=-i,n.lanes|=i,i=j_(i),T_(i,e,n,a),Ws(n,i),!1;break;case 22:if(n.memoizedState!==null)return n.flags|=65536,!1}n=n.return}while(n!==null);return!1}var No=Error(d(461)),Ze=!1;function Fe(e,t,n,a){t.child=e===null?qd(t,null,n,a):ga(t,e.child,n,a)}function C_(e,t,n,a,i){n=n.render;var l=t.ref;if("ref"in a){var s={};for(var u in a)u!=="ref"&&(s[u]=a[u])}else s=a;return fa(t),a=lo(e,t,n,s,l,i),u=ro(),e!==null&&!Ze?(so(e,t,i),Sn(e,t,i)):(oe&&u&&Ql(t),t.flags|=1,Fe(e,t,a,i),t.child)}function O_(e,t,n,a,i){if(e===null){var l=n.type;return typeof l=="function"&&!Bs(l)&&l.defaultProps===void 0&&n.compare===null?(t.tag=15,t.type=l,A_(e,t,l,a,i)):(e=Yl(n.type,null,a,t,t.mode,i),e.ref=t.ref,e.return=t,t.child=e)}if(l=e.child,!qo(e,i)){var s=l.memoizedProps;if(n=n.compare,n=n!==null?n:Di,n(s,a)&&e.ref===t.ref)return Sn(e,t,i)}return t.flags|=1,e=gn(l,a),e.ref=t.ref,e.return=t,t.child=e}function A_(e,t,n,a,i){if(e!==null){var l=e.memoizedProps;if(Di(l,a)&&e.ref===t.ref)if(Ze=!1,t.pendingProps=a=l,qo(e,i))(e.flags&131072)!==0&&(Ze=!0);else return t.lanes=e.lanes,Sn(e,t,i)}return So(e,t,n,a,i)}function q_(e,t,n,a){var i=a.children,l=e!==null?e.memoizedState:null;if(e===null&&t.stateNode===null&&(t.stateNode={_visibility:1,_pendingMarkers:null,_retryCache:null,_transitions:null}),a.mode==="hidden"){if((t.flags&128)!==0){if(l=l!==null?l.baseLanes|n:n,e!==null){for(a=t.child=e.child,i=0;a!==null;)i=i|a.lanes|a.childLanes,a=a.sibling;a=i&~l}else a=0,t.child=null;return R_(e,t,l,n,a)}if((n&536870912)!==0)t.memoizedState={baseLanes:0,cachePool:null},e!==null&&Fl(t,l!==null?l.cachePool:null),l!==null?Dd(t,l):eo(),Md(t);else return a=t.lanes=536870912,R_(e,t,l!==null?l.baseLanes|n:n,n,a)}else l!==null?(Fl(t,l.cachePool),Dd(t,l),In(),t.memoizedState=null):(e!==null&&Fl(t,null),eo(),In());return Fe(e,t,i,n),t.child}function Fi(e,t){return e!==null&&e.tag===22||t.stateNode!==null||(t.stateNode={_visibility:1,_pendingMarkers:null,_retryCache:null,_transitions:null}),t.sibling}function R_(e,t,n,a,i){var l=Zs();return l=l===null?null:{parent:Ke._currentValue,pool:l},t.memoizedState={baseLanes:n,cachePool:l},e!==null&&Fl(t,null),eo(),Md(t),e!==null&&ma(e,t,a,!0),t.childLanes=i,null}function mr(e,t){return t=fr({mode:t.mode,children:t.children},e.mode),t.ref=e.ref,e.child=t,t.return=e,t}function z_(e,t,n){return ga(t,e.child,null,n),e=mr(t,t.pendingProps),e.flags|=2,At(t),t.memoizedState=null,e}function V0(e,t,n){var a=t.pendingProps,i=(t.flags&128)!==0;if(t.flags&=-129,e===null){if(oe){if(a.mode==="hidden")return e=mr(t,a),t.lanes=536870912,e.memoizedState={baseLanes:0,cachePool:null},Fi(null,e);if(no(t),(e=Re)?(e=sf(e,Yt),e=e!==null&&e.data==="&"?e:null,e!==null&&(t.memoizedState={dehydrated:e,treeContext:zn!==null?{id:nn,overflow:an}:null,retryLane:536870912,hydrationErrors:null},n=hd(e),n.return=t,t.child=n,tt=t,Re=null)):e=null,e===null)throw Mn(t);return t.lanes=536870912,null}return mr(t,a)}var l=e.memoizedState;if(l!==null){var s=l.dehydrated;if(no(t),i)if(t.flags&256)t.flags&=-257,t=z_(e,t,n);else if(t.memoizedState!==null)t.child=e.child,t.flags|=128,t=null;else throw Error(d(558));else if(Ze||ma(e,t,n,!1),i=(n&e.childLanes)!==0,Ze||i){if(Gn.current===null){if(a=qe,a!==null&&(s=xu(a,n),s!==0&&s!==l.retryLane))throw l.retryLane=s,ca(e,s),St(a,e,s),No;Rr()}t=z_(e,t,n)}else e=l.treeContext,Re=Qt(s.nextSibling),tt=t,oe=!0,Dn=null,Yt=!1,e!==null&&xd(t,e),t=mr(t,a),t.flags|=134221824;return t}return e=gn(e.child,{mode:a.mode,children:a.children}),e.ref=t.ref,t.child=e,e.return=t,e}function Ja(e,t){var n=t.ref;if(n===null)e!==null&&e.ref!==null&&(t.flags|=4194816);else{if(typeof n!="function"&&typeof n!="object")throw Error(d(284));(e===null||e.ref!==n)&&(t.flags|=4194816)}}function So(e,t,n,a,i){return fa(t),n=lo(e,t,n,a,void 0,i),a=ro(),e!==null&&!Ze?(so(e,t,i),Sn(e,t,i)):(oe&&a&&Ql(t),t.flags|=1,Fe(e,t,n,i),t.child)}function D_(e,t,n,a,i,l){return fa(t),t.updateQueue=null,n=Bd(t,a,n,i),Ud(e),a=ro(),e!==null&&!Ze?(so(e,t,l),Sn(e,t,l)):(oe&&a&&Ql(t),t.flags|=1,Fe(e,t,n,l),t.child)}function M_(e,t,n,a,i){if(fa(t),t.stateNode===null){var l=Ha,s=n.contextType;typeof s=="object"&&s!==null&&(l=rt(s)),l=new n(a,l),t.memoizedState=l.state!==null&&l.state!==void 0?l.state:null,l.updater=wo,t.stateNode=l,l._reactInternals=t,l=t.stateNode,l.props=a,l.state=t.memoizedState,l.refs={},Fs(t),s=n.contextType,l.context=typeof s=="object"&&s!==null?rt(s):Ha,l.state=t.memoizedState,s=n.getDerivedStateFromProps,typeof s=="function"&&(bo(t,n,s,a),l.state=t.memoizedState),typeof n.getDerivedStateFromProps=="function"||typeof l.getSnapshotBeforeUpdate=="function"||typeof l.UNSAFE_componentWillMount!="function"&&typeof l.componentWillMount!="function"||(s=l.state,typeof l.componentWillMount=="function"&&l.componentWillMount(),typeof l.UNSAFE_componentWillMount=="function"&&l.UNSAFE_componentWillMount(),s!==l.state&&wo.enqueueReplaceState(l,l.state,null),Qi(t,a,l,i),Ii(),l.state=t.memoizedState),typeof l.componentDidMount=="function"&&(t.flags|=4194308),a=!0}else if(e===null){l=t.stateNode;var u=t.memoizedProps,_=ba(n,u);l.props=_;var g=l.context,w=n.contextType;s=Ha,typeof w=="object"&&w!==null&&(s=rt(w));var j=n.getDerivedStateFromProps;w=typeof j=="function"||typeof l.getSnapshotBeforeUpdate=="function",u=t.pendingProps!==u,w||typeof l.UNSAFE_componentWillReceiveProps!="function"&&typeof l.componentWillReceiveProps!="function"||(u||g!==s)&&w_(t,l,a,s),Vn=!1;var v=t.memoizedState;l.state=v,Qi(t,a,l,i),Ii(),g=t.memoizedState,u||v!==g||Vn?(typeof j=="function"&&(bo(t,n,j,a),g=t.memoizedState),(_=Vn||b_(t,n,_,a,v,g,s))?(w||typeof l.UNSAFE_componentWillMount!="function"&&typeof l.componentWillMount!="function"||(typeof l.componentWillMount=="function"&&l.componentWillMount(),typeof l.UNSAFE_componentWillMount=="function"&&l.UNSAFE_componentWillMount()),typeof l.componentDidMount=="function"&&(t.flags|=4194308)):(typeof l.componentDidMount=="function"&&(t.flags|=4194308),t.memoizedProps=a,t.memoizedState=g),l.props=a,l.state=g,l.context=s,a=_):(typeof l.componentDidMount=="function"&&(t.flags|=4194308),a=!1)}else{l=t.stateNode,Js(e,t),s=t.memoizedProps,w=ba(n,s),l.props=w,j=t.pendingProps,v=l.context,g=n.contextType,_=Ha,typeof g=="object"&&g!==null&&(_=rt(g)),u=n.getDerivedStateFromProps,(g=typeof u=="function"||typeof l.getSnapshotBeforeUpdate=="function")||typeof l.UNSAFE_componentWillReceiveProps!="function"&&typeof l.componentWillReceiveProps!="function"||(s!==j||v!==_)&&w_(t,l,a,_),Vn=!1,v=t.memoizedState,l.state=v,Qi(t,a,l,i),Ii();var b=t.memoizedState;s!==j||v!==b||Vn||e!==null&&e.dependencies!==null&&Zl(e.dependencies)?(typeof u=="function"&&(bo(t,n,u,a),b=t.memoizedState),(w=Vn||b_(t,n,w,a,v,b,_)||e!==null&&e.dependencies!==null&&Zl(e.dependencies))?(g||typeof l.UNSAFE_componentWillUpdate!="function"&&typeof l.componentWillUpdate!="function"||(typeof l.componentWillUpdate=="function"&&l.componentWillUpdate(a,b,_),typeof l.UNSAFE_componentWillUpdate=="function"&&l.UNSAFE_componentWillUpdate(a,b,_)),typeof l.componentDidUpdate=="function"&&(t.flags|=4),typeof l.getSnapshotBeforeUpdate=="function"&&(t.flags|=1024)):(typeof l.componentDidUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=4),typeof l.getSnapshotBeforeUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=1024),t.memoizedProps=a,t.memoizedState=b),l.props=a,l.state=b,l.context=_,a=w):(typeof l.componentDidUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=4),typeof l.getSnapshotBeforeUpdate!="function"||s===e.memoizedProps&&v===e.memoizedState||(t.flags|=1024),a=!1)}return l=a,Ja(e,t),a=(t.flags&128)!==0,l||a?(l=t.stateNode,n=a&&typeof n.getDerivedStateFromError!="function"?null:l.render(),t.flags|=1,e!==null&&a?(t.child=ga(t,e.child,null,i),t.child=ga(t,null,n,i)):Fe(e,t,n,i),t.memoizedState=l.state,e=t.child):e=Sn(e,t,i),e}function U_(e,t,n,a){return da(),t.flags|=256,Fe(e,t,n,a),t.child}var Eo={dehydrated:null,treeContext:null,retryLane:0,hydrationErrors:null};function jo(e){return{baseLanes:e,cachePool:Ed()}}function To(e,t,n){return e=e!==null?e.childLanes&~n:0,t&&(e|=zt),e}function B_(e,t,n){var a=t.pendingProps,i=!1,l=(t.flags&128)!==0,s;if((s=l)||(s=e!==null&&e.memoizedState===null?!1:(ot.current&2)!==0),s&&(i=!0,t.flags&=-129),s=(t.flags&32)!==0,t.flags&=-33,e===null){if(oe){if(i?Yn(t):In(),(e=Re)?(e=sf(e,Yt),e=e!==null&&e.data!=="&"?e:null,e!==null&&(t.memoizedState={dehydrated:e,treeContext:zn!==null?{id:nn,overflow:an}:null,retryLane:536870912,hydrationErrors:null},n=hd(e),n.return=t,t.child=n,tt=t,Re=null)):e=null,e===null)throw Mn(t);return jc(e)?t.lanes=32:t.lanes=536870912,null}return l=a.children,a=a.fallback,i?(In(),i=t.mode,l=fr({mode:"hidden",children:l},i),a=ua(a,i,n,null),l.return=t,a.return=t,l.sibling=a,t.child=l,a=t.child,a.memoizedState=jo(n),a.childLanes=To(e,s,n),t.memoizedState=Eo,Fi(null,a)):(Yn(t),Co(t,l))}var u=e.memoizedState;if(u!==null){var _=u.dehydrated;if(_!==null)return L0(e,t,l,s,a,_,u,n)}return i?(In(),i=a.fallback,l=t.mode,u=e.child,_=u.sibling,a=gn(u,{mode:"hidden",children:a.children}),a.subtreeFlags=u.subtreeFlags&1206910976,_!==null?i=gn(_,i):(i=ua(i,l,n,null),i.flags|=2),i.return=t,a.return=t,a.sibling=i,t.child=a,Fi(null,a),a=t.child,i=e.child.memoizedState,i===null?i=jo(n):(l=i.cachePool,l!==null?(u=Ke._currentValue,l=l.parent!==u?{parent:u,pool:u}:l):l=Ed(),i={baseLanes:i.baseLanes|n,cachePool:l}),a.memoizedState=i,a.childLanes=To(e,s,n),t.memoizedState=Eo,Fi(e.child,a)):(Yn(t),n=e.child,e=n.sibling,n=gn(n,{mode:"visible",children:a.children}),n.return=t,n.sibling=null,e!==null&&(s=t.deletions,s===null?(t.deletions=[e],t.flags|=16):s.push(e)),t.child=n,t.memoizedState=null,n)}function Co(e,t){return t=fr({mode:"visible",children:t},e.mode),t.return=e,e.child=t}function fr(e,t){return e=bt(22,e,null,t),e.lanes=0,e}function pr(e,t,n){return ga(t,e.child,null,n),e=Co(t,t.pendingProps.children),e.flags|=2,t.memoizedState=null,e}function L0(e,t,n,a,i,l,s,u){if(n)return t.flags&256?(Yn(t),t.flags&=-257,pr(e,t,u)):t.memoizedState!==null?(In(),t.child=e.child,t.flags|=128,null):(In(),l=i.fallback,s=t.mode,i=fr({mode:"visible",children:i.children},s),l=ua(l,s,u,null),l.flags|=2,i.return=t,l.return=t,i.sibling=l,t.child=i,ga(t,e.child,null,u),i=t.child,i.memoizedState=jo(u),i.childLanes=To(e,a,u),t.memoizedState=Eo,Fi(null,i));if(Yn(t),jc(l)){if(a=l.nextSibling&&l.nextSibling.dataset,a)var _=a.dgst;return a=_,a!==""&&(i=Error(d(419)),i.stack="",i.digest=a,Bi({value:i,source:null,stack:null})),pr(e,t,u)}if(Ze||ma(e,t,u,!1),a=(u&e.childLanes)!==0,Ze||a){if(Gn.current!==null)return pr(e,t,u);if(a=qe,a!==null&&(i=xu(a,u),i!==0&&i!==s.retryLane))throw s.retryLane=i,ca(e,i),St(a,e,i),No;return Ec(l)||Rr(),pr(e,t,u)}return Ec(l)?(t.flags|=192,t.child=e.child,null):(e=s.treeContext,Re=Qt(l.nextSibling),tt=t,oe=!0,Dn=null,Yt=!1,e!==null&&xd(t,e),t=Co(t,i.children),t.flags|=134221824,t)}function V_(e,t,n){e.lanes|=t;var a=e.alternate;a!==null&&(a.lanes|=t),Xl(e.return,t,n)}function L_(e){for(var t=null;e!==null;){var n=e.alternate;n!==null&&nr(n)===null&&(t=e),e=e.sibling}return t}function vr(e,t,n,a,i,l){var s=e.memoizedState;s===null?e.memoizedState={isBackwards:t,rendering:null,renderingStartTime:0,last:a,tail:n,tailMode:i,treeForkCount:l}:(s.isBackwards=t,s.rendering=null,s.renderingStartTime=0,s.last=a,s.tail=n,s.tailMode=i,s.treeForkCount=l)}function Oo(e){var t=e.child;for(e.child=null;t!==null;){var n=t.sibling;t.sibling=e.child,e.child=t,t=n}}function Ao(e,t,n){var a=t.pendingProps,i=a.revealOrder,l=a.tail;a=a.children;var s=ot.current;if(t.flags&128)return Ki(t,s),null;var u=(s&2)!==0;if(u?(s=s&1|2,t.flags|=128):s&=1,Ki(t,s),i==="backwards"&&e!==null?(Oo(e),Fe(e,t,a,n),Oo(e)):Fe(e,t,a,n),a=oe?Ui:0,!u&&e!==null&&(e.flags&128)!==0)e:for(e=t.child;e!==null;){if(e.tag===13)e.memoizedState!==null&&V_(e,n,t);else if(e.tag===19)V_(e,n,t);else if(e.child!==null){e.child.return=e,e=e.child;continue}if(e===t)break e;for(;e.sibling===null;){if(e.return===null||e.return===t)break e;e=e.return}e.sibling.return=e.return,e=e.sibling}switch(i){case"backwards":n=L_(t.child),n===null?(i=t.child,t.child=null):(i=n.sibling,n.sibling=null,Oo(t)),vr(t,!0,i,null,l,a);break;case"unstable_legacy-backwards":for(n=null,i=t.child,t.child=null;i!==null;){if(e=i.alternate,e!==null&&nr(e)===null){t.child=i;break}e=i.sibling,i.sibling=n,n=i,i=e}vr(t,!0,n,null,l,a);break;case"together":vr(t,!1,null,null,void 0,a);break;case"independent":t.memoizedState=null;break;default:n=L_(t.child),n===null?(i=t.child,t.child=null):(i=n.sibling,n.sibling=null),vr(t,!1,i,n,l,a)}return t.child}function H_(e,t,n){var a=t.pendingProps;return Un(t,t.type,a.value),Fe(e,t,a.children,n),t.child}function Sn(e,t,n){if(e!==null&&(t.dependencies=e.dependencies),Zn|=t.lanes,(n&t.childLanes)===0)if(e!==null){if(ma(e,t,n,!1),(n&t.childLanes)===0)return null}else return null;if(e!==null&&t.child!==e.child)throw Error(d(153));if(t.child!==null){for(e=t.child,n=gn(e,e.pendingProps),t.child=n,n.return=t;e.sibling!==null;)e=e.sibling,n=n.sibling=gn(e,e.pendingProps),n.return=t;n.sibling=null}return t.child}function qo(e,t){return(e.lanes&t)!==0?!0:(e=e.dependencies,!!(e!==null&&Zl(e)))}function H0(e,t,n){switch(t.tag){case 3:bl(t,t.stateNode.containerInfo),Un(t,Ke,e.memoizedState.cache),da();break;case 27:case 5:is(t);break;case 4:bl(t,t.stateNode.containerInfo);break;case 10:Un(t,t.type,t.memoizedProps.value);break;case 31:if(t.memoizedState!==null)return t.flags|=128,no(t),null;break;case 13:var a=t.memoizedState;if(a!==null){if(a.dehydrated!==null)return Yn(t),t.flags|=128,null;a=ma(e,t,n,!1);var i=t.child.childLanes;return a||(n&i)!==0?B_(e,t,n):(Yn(t),e=Sn(e,t,n),e!==null?e.sibling:null)}Yn(t);break;case 19:if(t.flags&128)return Ao(e,t,n);if(i=(e.flags&128)!==0,a=(n&t.childLanes)!==0,a||(ma(e,t,n,!1),a=(n&t.childLanes)!==0),i){if(a)return Ao(e,t,n);t.flags|=128}if(i=t.memoizedState,i!==null&&(i.rendering=null,i.tail=null,i.lastEffect=null),Ki(t,ot.current),a)break;return null;case 22:return t.lanes=0,q_(e,t,n,t.pendingProps);case 24:Un(t,Ke,e.memoizedState.cache)}return Sn(e,t,n)}function G_(e,t,n){if(e!==null)if(e.memoizedProps!==t.pendingProps)Ze=!0;else{if(!qo(e,n)&&(t.flags&128)===0)return Ze=!1,H0(e,t,n);Ze=(e.flags&131072)!==0}else Ze=!1,oe&&(t.flags&1048576)!==0&&gd(t,Ui,t.index);switch(t.lanes=0,t.tag){case 16:e:{var a=t.pendingProps;if(e=ha(t.elementType),t.type=e,typeof e=="function")Bs(e)?(a=ba(e,a),t.tag=1,t=M_(null,t,e,a,n)):(t.tag=0,t=So(null,t,e,a,n));else{if(e!=null){var i=e.$$typeof;if(i===D){t.tag=11,t=C_(null,t,e,a,n);break e}else if(i===ke){t.tag=14,t=O_(null,t,e,a,n);break e}else if(i===Ve){t.tag=10,t.type=e,t=H_(null,t,n);break e}}throw t=he(e)||e,Error(d(306,t,""))}}return t;case 0:return So(e,t,t.type,t.pendingProps,n);case 1:return a=t.type,i=ba(a,t.pendingProps),M_(e,t,a,i,n);case 3:e:{if(bl(t,t.stateNode.containerInfo),e===null)throw Error(d(387));a=t.pendingProps;var l=t.memoizedState;i=l.element,Js(e,t),Qi(t,a,null,n);var s=t.memoizedState;if(a=s.cache,Un(t,Ke,a),a!==l.cache&&Qs(t,[Ke],n,!0),Ii(),a=s.element,l.isDehydrated)if(l={element:a,isDehydrated:!1,cache:s.cache},t.updateQueue.baseState=l,t.memoizedState=l,t.flags&256){t=U_(e,t,a,n);break e}else if(a!==i){i=Lt(Error(d(424)),t),Bi(i),t=U_(e,t,a,n);break e}else{switch(e=t.stateNode.containerInfo,e.nodeType){case 9:e=e.body;break;default:e=e.nodeName==="HTML"?e.ownerDocument.body:e}for(Re=Qt(e.firstChild),tt=t,oe=!0,Dn=null,Yt=!0,n=qd(t,null,a,n),t.child=n;n;)n.flags=n.flags&-3|134221824,n=n.sibling}else{if(da(),a===i){t=Sn(e,t,n);break e}Fe(e,t,a,n)}t=t.child}return t;case 26:return Ja(e,t),e===null?(n=ff(t.type,null,t.pendingProps,null))?t.memoizedState=n:oe||(t.stateNode=Km(t.type,t.pendingProps,$e.current,t)):t.memoizedState=ff(t.type,e.memoizedProps,t.pendingProps,e.memoizedState),null;case 27:return is(t),e===null&&oe&&(a=t.stateNode=uf(t.type,t.pendingProps,$e.current),tt=t,Yt=!0,i=Re,Wn(t.type)?(Tc=i,Re=Qt(a.firstChild)):Re=i),Fe(e,t,t.pendingProps.children,n),Ja(e,t),e===null&&(t.flags|=4194304),t.child;case 5:return e===null&&oe&&((i=a=Re)&&(a=Dv(a,t.type,t.pendingProps,Yt),a!==null?(t.stateNode=a,tt=t,Re=Qt(a.firstChild),Yt=!1,i=!0):i=!1),i||Mn(t)),is(t),i=t.type,l=t.pendingProps,s=e!==null?e.memoizedProps:null,a=l.children,gc(i,l)?a=null:s!==null&&gc(i,s)&&(t.flags|=32),t.memoizedState!==null&&(i=lo(e,t,O0,null,null,n),pi._currentValue=i),Ja(e,t),Fe(e,t,a,n),t.child;case 6:return e===null&&oe&&((e=n=Re)&&(n=Mv(n,t.pendingProps,Yt),n!==null?(t.stateNode=n,tt=t,Re=null,e=!0):e=!1),e||Mn(t)),null;case 13:return B_(e,t,n);case 4:return bl(t,t.stateNode.containerInfo),a=t.pendingProps,e===null?t.child=ga(t,null,a,n):Fe(e,t,a,n),t.child;case 11:return C_(e,t,t.type,t.pendingProps,n);case 7:return a=t.pendingProps,Ja(e,t),Fe(e,t,a,n),t.child;case 8:return Fe(e,t,t.pendingProps.children,n),t.child;case 12:return Fe(e,t,t.pendingProps.children,n),t.child;case 10:return H_(e,t,n);case 9:return i=t.type._context,a=t.pendingProps.children,fa(t),i=rt(i),a=a(i),t.flags|=1,Fe(e,t,a,n),t.child;case 14:return O_(e,t,t.type,t.pendingProps,n);case 15:return A_(e,t,t.type,t.pendingProps,n);case 19:return Ao(e,t,n);case 31:return V0(e,t,n);case 22:return q_(e,t,n,t.pendingProps);case 24:return fa(t),a=rt(Ke),e===null?(i=Zs(),i===null&&(i=qe,l=Ks(),i.pooledCache=l,l.refCount++,l!==null&&(i.pooledCacheLanes|=n),i=l),t.memoizedState={parent:a,cache:i},Fs(t),Un(t,Ke,i)):((e.lanes&n)!==0&&(Js(e,t),Qi(t,null,null,n),Ii()),i=e.memoizedState,l=t.memoizedState,i.parent!==a?(i={parent:a,cache:a},t.memoizedState=i,t.lanes===0&&(t.memoizedState=t.updateQueue.baseState=i),Un(t,Ke,a)):(a=l.cache,Un(t,Ke,a),a!==i.cache&&Qs(t,[Ke],n,!0))),Fe(e,t,t.pendingProps.children,n),t.child;case 30:return t.stateNode===null&&(t.stateNode={autoName:null,paired:null,clones:null,ref:null}),a=t.pendingProps,a.name!=null&&a.name!=="auto"?t.flags|=e===null?18882560:18874368:oe&&Ql(t),e!==null&&e.memoizedProps.name!==a.name?t.flags|=4194816:Ja(e,t),Fe(e,t,a.children,n),t.child;case 29:throw t.pendingProps}throw Error(d(156,t.tag))}function En(e){e.flags|=4}function Ro(e,t,n,a,i){var l;if((l=(e.mode&32)!==0)&&(l=n===null?yf(t,a):yf(t,a)&&(a.src!==n.src||a.srcSet!==n.srcSet)),l){if(e.flags|=16777216,(i&335544128)===i)if(e.stateNode.complete)e.flags|=8192;else if(km())e.flags|=8192;else throw ya=Wl,Ps}else e.flags&=-16777217}function Y_(e,t){if(t.type!=="stylesheet"||(t.state.loading&4)!==0)e.flags&=-16777217;else if(e.flags|=16777216,!gf(t))if(km())e.flags|=8192;else throw ya=Wl,Ps}function hr(e,t){t!==null&&(e.flags|=4),e.flags&16384&&(t=e.tag!==22?hu():536870912,e.lanes|=t,ni|=t)}function Ji(e,t){if(!oe)switch(e.tailMode){case"visible":break;case"collapsed":for(var n=e.tail,a=null;n!==null;)n.alternate!==null&&(a=n),n=n.sibling;a===null?t||e.tail===null?e.tail=null:e.tail.sibling=null:a.sibling=null;break;default:for(t=e.tail,n=null;t!==null;)t.alternate!==null&&(n=t),t=t.sibling;n===null?e.tail=null:n.sibling=null}}function ze(e){var t=e.alternate!==null&&e.alternate.child===e.child,n=0,a=0;if(t)for(var i=e.child;i!==null;)n|=i.lanes|i.childLanes,a|=i.subtreeFlags&1206910976,a|=i.flags&1206910976,i.return=e,i=i.sibling;else for(i=e.child;i!==null;)n|=i.lanes|i.childLanes,a|=i.subtreeFlags,a|=i.flags,i.return=e,i=i.sibling;return e.subtreeFlags|=a,e.childLanes=n,t}function G0(e,t,n){var a=t.pendingProps;switch(Hs(t),t.tag){case 16:case 15:case 0:case 11:case 7:case 8:case 12:case 9:case 14:return ze(t),null;case 1:return ze(t),null;case 3:return n=t.stateNode,a=null,e!==null&&(a=e.memoizedState.cache),t.memoizedState.cache!==a&&(t.flags|=2048),wn(Ke),Ta(),n.pendingContext&&(n.context=n.pendingContext,n.pendingContext=null),(e===null||e.child===null)&&(Ia(t)?En(t):e===null||e.memoizedState.isDehydrated&&(t.flags&256)===0||(t.flags|=1024,Ys())),ze(t),null;case 26:var i=t.type,l=t.memoizedState;return e===null?(En(t),l!==null?(ze(t),Y_(t,l)):(ze(t),Ro(t,i,null,a,n))):l?l!==e.memoizedState?(En(t),ze(t),Y_(t,l)):(ze(t),t.flags&=-16777217):(e=e.memoizedProps,e!==a&&En(t),ze(t),Ro(t,i,e,a,n)),null;case 27:if(wl(t),n=$e.current,i=t.type,e!==null&&t.stateNode!=null)e.memoizedProps!==a&&En(t);else{if(!a){if(t.stateNode===null)throw Error(d(166));return ze(t),t.subtreeFlags&=-33554433,null}e=Ge.current,Ia(t)?bd(t):(e=uf(i,a,n),t.stateNode=e,En(t))}return ze(t),t.subtreeFlags&=-33554433,null;case 5:if(wl(t),i=t.type,e!==null&&t.stateNode!=null)e.memoizedProps!==a&&En(t);else{if(!a){if(t.stateNode===null)throw Error(d(166));return ze(t),t.subtreeFlags&=-33554433,null}if(l=Ge.current,Ia(t))bd(t);else{var s=ol($e.current);switch(l){case 1:l=s.createElementNS("http://www.w3.org/2000/svg",i);break;case 2:l=s.createElementNS("http://www.w3.org/1998/Math/MathML",i);break;default:switch(i){case"svg":l=s.createElementNS("http://www.w3.org/2000/svg",i);break;case"math":l=s.createElementNS("http://www.w3.org/1998/Math/MathML",i);break;case"script":l=s.createElement("div"),l.innerHTML="<script><\/script>",l=l.removeChild(l.firstChild);break;case"select":l=typeof a.is=="string"?s.createElement("select",{is:a.is}):s.createElement("select"),a.multiple?l.multiple=!0:a.size&&(l.size=a.size);break;default:l=typeof a.is=="string"?s.createElement(i,{is:a.is}):s.createElement(i)}}l[lt]=t,l[xt]=a;e:for(s=t.child;s!==null;){if(s.tag===5||s.tag===6)l.appendChild(s.stateNode);else if(s.tag!==4&&s.tag!==27&&s.child!==null){s.child.return=s,s=s.child;continue}if(s===t)break e;for(;s.sibling===null;){if(s.return===null||s.return===t)break e;s=s.return}s.sibling.return=s.return,s=s.sibling}t.stateNode=l;e:switch(ut(l,i,a),i){case"button":case"input":case"select":case"textarea":a=!!a.autoFocus;break e;case"img":a=!0;break e;default:a=!1}a&&En(t)}}return ze(t),t.subtreeFlags&=-33554433,Ro(t,t.type,e===null?null:e.memoizedProps,t.pendingProps,n),null;case 6:if(e&&t.stateNode!=null)e.memoizedProps!==a&&En(t);else{if(typeof a!="string"&&t.stateNode===null)throw Error(d(166));if(e=$e.current,Ia(t)){if(e=t.stateNode,n=t.memoizedProps,a=null,i=tt,i!==null)switch(i.tag){case 27:case 5:a=i.memoizedProps}e[lt]=t,e=!!(e.nodeValue===n||a!==null&&a.suppressHydrationWarning===!0||Gm(e.nodeValue,n)),e||Mn(t,!0)}else e=ol(e).createTextNode(a),e[lt]=t,t.stateNode=e}return ze(t),null;case 31:if(n=t.memoizedState,e===null||e.memoizedState!==null){if(a=Ia(t),n!==null){if(e===null){if(!a)throw Error(d(318));if(e=t.memoizedState,e=e!==null?e.dehydrated:null,!e)throw Error(d(557));e[lt]=t}else da(),(t.flags&128)===0&&(t.memoizedState=null),t.flags|=4;ze(t),e=!1}else n=Ys(),e!==null&&e.memoizedState!==null&&(e.memoizedState.hydrationErrors=n),e=!0;if(!e)return t.flags&256?(At(t),t):(At(t),null);if((t.flags&128)!==0)throw Error(d(558))}return ze(t),null;case 13:if(a=t.memoizedState,e===null||e.memoizedState!==null&&e.memoizedState.dehydrated!==null){if(i=Ia(t),a!==null&&a.dehydrated!==null){if(e===null){if(!i)throw Error(d(318));if(i=t.memoizedState,i=i!==null?i.dehydrated:null,!i)throw Error(d(317));i[lt]=t}else da(),(t.flags&128)===0&&(t.memoizedState=null),t.flags|=4;ze(t),i=!1}else i=Ys(),e!==null&&e.memoizedState!==null&&(e.memoizedState.hydrationErrors=i),i=!0;if(!i)return t.flags&256?(At(t),t):(At(t),null)}return At(t),(t.flags&128)!==0?(t.lanes=n,t):(n=a!==null,e=e!==null&&e.memoizedState!==null,n&&(a=t.child,i=null,a.alternate!==null&&a.alternate.memoizedState!==null&&a.alternate.memoizedState.cachePool!==null&&(i=a.alternate.memoizedState.cachePool.pool),l=null,a.memoizedState!==null&&a.memoizedState.cachePool!==null&&(l=a.memoizedState.cachePool.pool),l!==i&&(a.flags|=2048)),n!==e&&n&&(t.child.flags|=8192),hr(t,t.updateQueue),ze(t),null);case 4:return Ta(),e===null&&fc(t.stateNode.containerInfo),t.flags|=67108864,ze(t),null;case 10:return wn(t.type),ze(t),null;case 19:if(ao(t),a=t.memoizedState,a===null)return ze(t),null;if(i=(t.flags&128)!==0,l=a.rendering,l===null)if(i)Ji(a,!1);else{if(Ie!==0||e!==null&&(e.flags&128)!==0)for(e=t.child;e!==null;){if(l=nr(e),l!==null){for(t.flags|=128,Ji(a,!1),e=l.updateQueue,t.updateQueue=e,hr(t,e),t.subtreeFlags=0,e=n,n=t.child;n!==null;)vd(n,e),n=n.sibling;return Ki(t,ot.current&1|2),oe&&xn(t,a.treeForkCount),t.child}e=e.sibling}a.tail!==null&&Et()>Cr&&(t.flags|=128,i=!0,Ji(a,!1),t.lanes=4194304)}else{if(!i)if(e=nr(l),e!==null){if(t.flags|=128,i=!0,e=e.updateQueue,t.updateQueue=e,hr(t,e),Ji(a,!0),a.tail===null&&a.tailMode!=="collapsed"&&a.tailMode!=="visible"&&!l.alternate&&!oe)return ze(t),null}else 2*Et()-a.renderingStartTime>Cr&&n!==536870912&&(t.flags|=128,i=!0,Ji(a,!1),t.lanes=4194304);a.isBackwards?(l.sibling=t.child,t.child=l):(e=a.last,e!==null?e.sibling=l:t.child=l,a.last=l)}if(a.tail!==null){e=a.tail;e:{for(n=e;n!==null;){if(n.alternate!==null){n=!1;break e}n=n.sibling}n=!0}return a.rendering=e,a.tail=e.sibling,a.renderingStartTime=Et(),e.sibling=null,l=ot.current,l=i?l&1|2:l&1,a.tailMode==="visible"||a.tailMode==="collapsed"||!n||oe?Ki(t,l):(n=l,ce(st,t),ce(ot,n),mt===null&&(mt=t)),oe&&xn(t,a.treeForkCount),e}return ze(t),null;case 22:case 23:return At(t),to(),a=t.memoizedState!==null,e!==null?e.memoizedState!==null!==a&&(t.flags|=8192):a&&(t.flags|=8192),a?(n&536870912)!==0&&(t.flags&128)===0&&(ze(t),t.subtreeFlags&6&&(t.flags|=8192)):ze(t),n=t.updateQueue,n!==null&&hr(t,n.retryQueue),n=null,e!==null&&e.memoizedState!==null&&e.memoizedState.cachePool!==null&&(n=e.memoizedState.cachePool.pool),a=null,t.memoizedState!==null&&t.memoizedState.cachePool!==null&&(a=t.memoizedState.cachePool.pool),a!==n&&(t.flags|=2048),e!==null&&F(va),null;case 24:return n=null,e!==null&&(n=e.memoizedState.cache),t.memoizedState.cache!==n&&(t.flags|=2048),wn(Ke),ze(t),null;case 25:return null;case 30:return t.flags|=33554432,ze(t),null}throw Error(d(156,t.tag))}function Y0(e,t){switch(Hs(t),t.tag){case 1:return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 3:return wn(Ke),Ta(),e=t.flags,(e&65536)!==0&&(e&128)===0?(t.flags=e&-65537|128,t):null;case 26:case 27:case 5:return wl(t),null;case 31:if(t.memoizedState!==null){if(At(t),t.alternate===null)throw Error(d(340));da()}return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 13:if(At(t),e=t.memoizedState,e!==null&&e.dehydrated!==null){if(t.alternate===null)throw Error(d(340));da()}return e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 19:return ao(t),e=t.flags,e&65536?(t.flags=e&-65537|128,e=t.memoizedState,e!==null&&(e.rendering=null,e.tail=null),t.flags|=4,t):null;case 4:return Ta(),null;case 10:return wn(t.type),null;case 22:case 23:return At(t),to(),e!==null&&F(va),e=t.flags,e&65536?(t.flags=e&-65537|128,t):null;case 24:return wn(Ke),null;case 25:return null;default:return null}}function I_(e,t){switch(Hs(t),t.tag){case 3:wn(Ke),Ta();break;case 26:case 27:case 5:wl(t);break;case 4:Ta();break;case 31:t.memoizedState!==null&&At(t);break;case 13:At(t);break;case 19:ao(t);break;case 10:wn(t.type);break;case 22:case 23:At(t),to(),e!==null&&F(va);break;case 24:wn(Ke)}}function Wi(e,t){try{var n=t.updateQueue,a=n!==null?n.lastEffect:null;if(a!==null){var i=a.next;n=i;do{if((n.tag&e)===e){a=void 0;var l=n.create,s=n.inst;a=l(),s.destroy=a}n=n.next}while(n!==i)}}catch(u){Ce(t,t.return,u)}}function Qn(e,t,n){try{var a=t.updateQueue,i=a!==null?a.lastEffect:null;if(i!==null){var l=i.next;a=l;do{if((a.tag&e)===e){var s=a.inst,u=s.destroy;if(u!==void 0){s.destroy=void 0,i=t;var _=n,g=u;try{g()}catch(w){Ce(i,_,w)}}}a=a.next}while(a!==l)}}catch(w){Ce(t,t.return,w)}}function Q_(e){var t=e.updateQueue;if(t!==null){var n=e.stateNode;try{zd(t,n)}catch(a){Ce(e,e.return,a)}}}function K_(e,t,n){n.props=ba(e.type,e.memoizedProps),n.state=e.memoizedState;try{n.componentWillUnmount()}catch(a){Ce(e,t,a)}}function ln(e,t){try{var n=e.ref;if(n!==null){switch(e.tag){case 26:case 27:case 5:var a=e.stateNode;break;case 30:var i=e.stateNode,l=hn(e.memoizedProps,i);(i.ref===null||i.ref.name!==l)&&(i.ref=$m(l)),a=i.ref;break;case 7:if(e.stateNode===null){var s=new Mt(e);y(e.child,!1,Rv,s,void 0,void 0),e.stateNode=s}a=e.stateNode;break;default:a=e.stateNode}typeof n=="function"?e.refCleanup=n(a):n.current=a}}catch(u){Ce(e,t,u)}}function ct(e,t){var n=e.ref,a=e.refCleanup;if(n!==null)if(typeof a=="function")try{a()}catch(i){Ce(e,t,i)}finally{e.refCleanup=null,e=e.alternate,e!=null&&(e.refCleanup=null)}else if(typeof n=="function")try{n(null)}catch(i){Ce(e,t,i)}else n.current=null}function yr(e,t){if((e.tag===5||e.tag===27||e.tag===6)&&e.alternate===null&&t!==null)for(var n=0;n<t.length;n++)rf(e.stateNode,t[n])}function X_(e){for(var t=e.return;t!==null&&(Do(t)&&rf(e.stateNode,t.stateNode),!zo(t));)t=t.return}function $i(e){for(var t=e.return;t!==null&&(Do(t)&&zv(e.stateNode,t.stateNode),!zo(t));)t=t.return}function zo(e){return e.tag===5||e.tag===3||e.tag===27}function Do(e){return e&&e.tag===7&&e.stateNode!==null}function Mo(e){var t=e.type,n=e.memoizedProps,a=e.stateNode;try{e:switch(t){case"button":case"input":case"select":case"textarea":n.autoFocus&&a.focus();break e;case"img":n.src?a.src=n.src:n.srcSet&&(a.srcset=n.srcSet)}}catch(i){Ce(e,e.return,i)}}function Uo(e,t,n){try{var a=e.stateNode;pv(a,e.type,n,t),a[xt]=t}catch(i){Ce(e,e.return,i)}}function Z_(e){return e.tag===5||e.tag===3||e.tag===26||e.tag===27&&Wn(e.type)||e.tag===4}function Bo(e){e:for(;;){for(;e.sibling===null;){if(e.return===null||Z_(e.return))return null;e=e.return}for(e.sibling.return=e.return,e=e.sibling;e.tag!==5&&e.tag!==6&&e.tag!==18;){if(e.tag===27&&Wn(e.type)||e.flags&2||e.child===null||e.tag===4)continue e;e.child.return=e,e=e.child}if(!(e.flags&2))return e.stateNode}}function Vo(e,t,n,a){var i=e.tag;if(i===5||i===6)i=e.stateNode,t?(n.nodeType===9?n.body:n.nodeName==="HTML"?n.ownerDocument.body:n).insertBefore(i,t):(t=n.nodeType===9?n.body:n.nodeName==="HTML"?n.ownerDocument.body:n,t.appendChild(i),n=n._reactRootContainer,n!=null||t.onclick!==null||(t.onclick=tn)),yr(e,a),Ne=!0;else if(i!==4&&(i===27&&(yr(e,a),a=null,Wn(e.type)&&(n=e.stateNode,t=null)),e=e.child,e!==null))for(Vo(e,t,n,a),e=e.sibling;e!==null;)Vo(e,t,n,a),e=e.sibling}function gr(e,t,n,a){var i=e.tag;if(i===5||i===6)i=e.stateNode,t?n.insertBefore(i,t):n.appendChild(i),yr(e,a),Ne=!0;else if(i!==4&&(i===27&&(yr(e,a),a=null,Wn(e.type)&&(n=e.stateNode)),e=e.child,e!==null))for(gr(e,t,n,a),e=e.sibling;e!==null;)gr(e,t,n,a),e=e.sibling}function P_(e){var t=e.stateNode,n=e.memoizedProps;try{for(var a=e.type,i=t.attributes;i.length;)t.removeAttributeNode(i[0]);ut(t,a,n),t[lt]=e,t[xt]=n}catch(l){Ce(e,e.return,l)}}var xr=!1,qt=null;function F_(e){(e.tag===30||(e.subtreeFlags&33554432)!==0)&&(xr=!0)}var rn=null;function J_(){var e=rn;return rn=null,e}var wt=0;function Wa(e,t,n,a,i){return wt=0,W_(e.child,t,n,a,i)}function W_(e,t,n,a,i){for(var l=!1;e!==null;){if(e.tag===5){var s=e.stateNode;if(a!==null){var u=wc(s);a.push(u),u.view&&(l=!0)}else l||wc(s).view&&(l=!0);xr=!0,Jm(s,wt===0?t:t+"_"+wt,n),wt++}else(e.tag!==22||e.memoizedState===null)&&(e.tag===30&&i||W_(e.child,t,n,a,i)&&(l=!0));e=e.sibling}return l}function sn(e,t){for(;e!==null;)e.tag===5?Wm(e.stateNode,e.memoizedProps):(e.tag!==22||e.memoizedState===null)&&(e.tag===30&&t||sn(e.child,t)),e=e.sibling}function br(e){if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if((e.tag!==22||e.memoizedState===null)&&(br(e),e.tag===30&&(e.flags&18874368)!==0&&e.stateNode.paired)){var t=e.memoizedProps;if(t.name==null||t.name==="auto")throw Error(d(544));var n=t.name;t=yn(t.default,t.share),t!=="none"&&(Wa(e,n,t,null,!1)||sn(e.child,!1))}e=e.sibling}}function Lo(e,t){if(e.tag===30){var n=e.stateNode,a=e.memoizedProps,i=hn(a,n),l=yn(a.default,n.paired?a.share:a.enter);l!=="none"?Wa(e,i,l,null,!1)?(br(e),n.paired||t||ri(e,a.onEnter)):sn(e.child,!1):br(e)}else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)Lo(e,t),e=e.sibling;else br(e)}function Ho(e){if(qt!==null&&qt.size!==0){var t=qt;if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if(e.tag!==22||e.memoizedState===null){if(e.tag===30&&(e.flags&18874368)!==0){var n=e.memoizedProps,a=n.name;if(a!=null&&a!=="auto"){var i=t.get(a);if(i!==void 0){var l=yn(n.default,n.share);if(l!=="none"&&(Wa(e,a,l,null,!1)?(l=e.stateNode,i.paired=l,l.paired=i,ri(e,n.onShare)):sn(e.child,!1)),t.delete(a),t.size===0)break}}}Ho(e)}e=e.sibling}}}function Go(e){if(e.tag===30){var t=e.memoizedProps,n=hn(t,e.stateNode),a=qt!==null?qt.get(n):void 0,i=yn(t.default,a!==void 0?t.share:t.exit);i!=="none"&&(Wa(e,n,i,null,!1)?a!==void 0?(i=e.stateNode,a.paired=i,i.paired=a,qt.delete(n),ri(e,t.onShare)):ri(e,t.onExit):sn(e.child,!1)),qt!==null&&Ho(e)}else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)Go(e),e=e.sibling;else qt!==null&&Ho(e)}function $_(e){for(e=e.child;e!==null;){if(e.tag===30){var t=e.memoizedProps,n=hn(t,e.stateNode);t=yn(t.default,t.update),e.flags&=-5,t!=="none"&&Wa(e,n,t,e.memoizedState=[],!1)}else(e.subtreeFlags&33554432)!==0&&$_(e);e=e.sibling}}function Yo(e){if((e.subtreeFlags&18874368)!==0)for(e=e.child;e!==null;){if(e.tag!==22||e.memoizedState===null){if(e.tag===30&&(e.flags&18874368)!==0){var t=e.stateNode;t.paired!==null&&(t.paired=null,sn(e.child,!1))}Yo(e)}e=e.sibling}}function wr(e){if(e.tag===30)e.stateNode.paired=null,sn(e.child,!1),Yo(e);else if((e.subtreeFlags&33554432)!==0)for(e=e.child;e!==null;)wr(e),e=e.sibling;else Yo(e)}function em(e){for(e=e.child;e!==null;)e.tag===30?sn(e.child,!1):(e.subtreeFlags&33554432)!==0&&em(e),e=e.sibling}function Io(e,t,n,a,i,l,s){for(var u=!1;t!==null;){if(t.tag===5){var _=t.stateNode;if(l!==null&&wt<l.length){var g=l[wt],w=wc(_);(g.view||w.view)&&(u=!0);var j;if(j=(e.flags&4)===0)if(w.clip)j=!0;else{j=g.rect;var v=w.rect;j=j.y!==v.y||j.x!==v.x||j.height!==v.height||j.width!==v.width}j&&(e.flags|=4),w.abs?w=!g.abs:(g=g.rect,w=w.rect,w=g.height!==w.height||g.width!==w.width),w&&(e.flags|=32)}else e.flags|=32;(e.flags&4)!==0&&Jm(_,wt===0?n:n+"_"+wt,i),u&&(e.flags&4)!==0||(rn===null&&(rn=[]),rn.push(_,wt===0?a:a+"_"+wt,t.memoizedProps)),wt++}else(t.tag!==22||t.memoizedState===null)&&(t.tag===30&&s?e.flags|=t.flags&32:Io(e,t.child,n,a,i,l,s)&&(u=!0));t=t.sibling}return u}function tm(e,t){for(e=e.child;e!==null;){if(e.tag===30){var n=e.memoizedProps,a=e.stateNode,i=hn(n,a),l=yn(n.default,n.update),s;s=e.memoizedState,e.memoizedState=null,a=e;var u=e.child;wt=0,i=Io(a,u,i,i,l,s,!1),(e.flags&4)!==0&&i&&ri(e,n.onUpdate)}else(e.subtreeFlags&33554432)!==0&&tm(e);e=e.sibling}}var nt=!1,je=!1,on=!1,Qo=!1,nm=typeof WeakSet=="function"?WeakSet:Set,at=null,cn=!1,el=!1,kr=!1,Ko=!1;function I0(e,t,n){if(e=e.containerInfo,hc=vi,e=rd(e),As(e)){if("selectionStart"in e)var a={start:e.selectionStart,end:e.selectionEnd};else e:{a=(a=e.ownerDocument)&&a.defaultView||window;var i=a.getSelection&&a.getSelection();if(i&&i.rangeCount!==0){a=i.anchorNode;var l=i.anchorOffset,s=i.focusNode;i=i.focusOffset;try{a.nodeType,s.nodeType}catch{a=null;break e}var u=0,_=-1,g=-1,w=0,j=0,v=e,b=null;t:for(;;){for(var B;v!==a||l!==0&&v.nodeType!==3||(_=u+l),v!==s||i!==0&&v.nodeType!==3||(g=u+i),v.nodeType===3&&(u+=v.nodeValue.length),(B=v.firstChild)!==null;)b=v,v=B;for(;;){if(v===e)break t;if(b===a&&++w===l&&(_=u),b===s&&++j===i&&(g=u),(B=v.nextSibling)!==null)break;v=b,b=v.parentNode}v=B}a=_===-1||g===-1?null:{start:_,end:g}}else a=null}a=a||{start:0,end:0}}else a=null;for(yc={focusedElem:e,selectionRange:a},vi=!1,n=(n&335544064)===n,at=t,t=n?9270:1024;at!==null;){if(e=at,n&&(a=e.deletions,a!==null))for(l=0;l<a.length;l++)n&&Go(a[l]);if(e.alternate===null&&(e.flags&2)!==0)n&&F_(e),Nr(n);else{if(e.tag===22){if(a=e.alternate,e.memoizedState!==null){a!==null&&a.memoizedState===null&&n&&Go(a),Nr(n);continue}else if(a!==null&&a.memoizedState!==null){n&&F_(e),Nr(n);continue}}a=e.child,(e.subtreeFlags&t)!==0&&a!==null?(a.return=e,at=a):(n&&$_(e),Nr(n))}}qt=null}function Nr(e){for(;at!==null;){var t=at,n=e,a=t.alternate,i=t.flags;switch(t.tag){case 0:case 11:case 15:break;case 1:if((i&1024)!==0&&a!==null){n=void 0,i=a.memoizedProps,a=a.memoizedState;var l=t.stateNode;try{var s=ba(t.type,i);n=l.getSnapshotBeforeUpdate(s,a),l.__reactInternalSnapshotBeforeUpdate=n}catch(u){Ce(t,t.return,u)}}break;case 3:if((i&1024)!==0){if(a=t.stateNode.containerInfo,n=a.nodeType,n===9)Sc(a);else if(n===1)switch(a.nodeName){case"HEAD":case"HTML":case"BODY":Sc(a);break;default:a.textContent=""}}break;case 5:case 26:case 27:case 6:case 4:case 17:break;case 30:n&&a!==null&&(n=hn(a.memoizedProps,a.stateNode),i=t.memoizedProps,i=yn(i.default,i.update),i!=="none"&&Wa(a,n,i,a.memoizedState=[],!0));break;default:if((i&1024)!==0)throw Error(d(163))}if(a=t.sibling,a!==null){a.return=t.return,at=a;break}at=t.return}}function am(e,t,n){var a=n.flags;switch(n.tag){case 0:case 11:case 15:un(e,n),a&4&&Wi(5,n);break;case 1:if(un(e,n),a&4)if(e=n.stateNode,t===null)try{e.componentDidMount()}catch(s){Ce(n,n.return,s)}else{var i=ba(n.type,t.memoizedProps);t=t.memoizedState;try{e.componentDidUpdate(i,t,e.__reactInternalSnapshotBeforeUpdate)}catch(s){Ce(n,n.return,s)}}a&64&&Q_(n),a&512&&ln(n,n.return);break;case 3:if(un(e,n),a&64&&(e=n.updateQueue,e!==null)){if(t=null,n.child!==null)switch(n.child.tag){case 27:case 5:t=n.child.stateNode;break;case 1:t=n.child.stateNode}try{zd(e,t)}catch(s){Ce(n,n.return,s)}}break;case 27:t===null&&a&4&&P_(n);case 26:case 5:un(e,n),t===null&&a&4&&Mo(n),a&512&&ln(n,n.return);break;case 12:un(e,n);break;case 31:un(e,n),a&4&&sm(e,n);break;case 13:un(e,n),a&4&&om(e,n),a&64&&(e=n.memoizedState,e!==null&&(e=e.dehydrated,e!==null&&(n=nv.bind(null,n),Uv(e,n))));break;case 22:if(a=n.memoizedState!==null||nt,!a){var l=t!==null&&t.memoizedState!==null||je;t=nt,i=je,nt=a,(je=l)&&!i?(a=2,(n.subtreeFlags&8772)!==0&&(a|=1),Jt(e,n,a)):un(e,n),nt=t,je=i}break;case 30:un(e,n),a&512&&ln(n,n.return);break;case 7:a&512&&ln(n,n.return);default:un(e,n)}}function Xo(e,t){for(e=e.child;e!==null;)im(e,t),e=e.sibling}function im(e,t){switch(e.tag){case 5:case 26:try{var n=e.stateNode;if(t){var a=n.style;typeof a.setProperty=="function"?a.setProperty("display","none","important"):a.display="none"}else{var i=e.stateNode,l=e.memoizedProps.style,s=l!=null&&l.hasOwnProperty("display")?l.display:null;i.style.display=s==null||typeof s=="boolean"?"":(""+s).trim()}}catch(_){Ce(e,e.return,_)}Zo(e,t);break;case 6:try{e.stateNode.nodeValue=t?"":e.memoizedProps,Ne=!0}catch(_){Ce(e,e.return,_)}break;case 18:try{var u=e.stateNode;t?Fm(u,!0):Fm(e.stateNode,!1)}catch(_){Ce(e,e.return,_)}break;case 22:case 23:e.memoizedState===null&&Xo(e,t);break;default:Xo(e,t)}}function Zo(e,t){if(e.subtreeFlags&67108864)for(e=e.child;e!==null;){e:{var n=e,a=t;switch(n.tag){case 4:im(n,a);break e;case 22:n.memoizedState===null&&Zo(n,a);break e;default:Zo(n,a)}}e=e.sibling}}function lm(e){var t=e.alternate;t!==null&&(e.alternate=null,lm(t)),e.child=null,e.deletions=null,e.sibling=null,e.tag===5&&(t=e.stateNode,t!==null&&Cl(t)),e.stateNode=null,e.return=null,e.dependencies=null,e.memoizedProps=null,e.memoizedState=null,e.pendingProps=null,e.stateNode=null,e.updateQueue=null}var Ue=null,kt=!1;function Pt(e,t,n){for(n=n.child;n!==null;)rm(e,t,n),n=n.sibling}function rm(e,t,n){if(jt&&typeof jt.onCommitFiberUnmount=="function")try{jt.onCommitFiberUnmount(ki,n)}catch{}switch(n.tag){case 26:je||ct(n,t),Pt(e,t,n),n.memoizedState?n.memoizedState.count--:n.stateNode&&!je&&(n=n.stateNode,n.parentNode.removeChild(n));break;case 27:je||ct(n,t),$i(n);var a=Ue,i=kt;Wn(n.type)&&(Ue=n.stateNode,kt=!1),Pt(e,t,n),df(n.stateNode,n.type,n.memoizedProps),Ue=a,kt=i;break;case 5:je||ct(n,t),$i(n);case 6:if(n.tag===6&&$i(n),a=Ue,i=kt,Ue=null,Pt(e,t,n),Ue=a,kt=i,Ue!==null)if(kt)try{(Ue.nodeType===9?Ue.body:Ue.nodeName==="HTML"?Ue.ownerDocument.body:Ue).removeChild(n.stateNode),Ne=!0}catch(l){Ce(n,t,l)}else try{Ue.removeChild(n.stateNode),Ne=!0}catch(l){Ce(n,t,l)}break;case 18:Ue!==null&&(kt?(e=Ue,Pm(e.nodeType===9?e.body:e.nodeName==="HTML"?e.ownerDocument.body:e,n.stateNode),hi(e)):Pm(Ue,n.stateNode));break;case 4:a=Ue,i=kt,Ue=n.stateNode.containerInfo,kt=!0,Pt(e,t,n),Ue=a,kt=i;break;case 0:case 11:case 14:case 15:Qn(2,n,t),je||Qn(4,n,t),Pt(e,t,n);break;case 1:je||(ct(n,t),a=n.stateNode,typeof a.componentWillUnmount=="function"&&K_(n,t,a)),Pt(e,t,n);break;case 21:Pt(e,t,n);break;case 22:je=(a=je)||n.memoizedState!==null,Pt(e,t,n),je=a;break;case 30:ct(n,t),Pt(e,t,n);break;case 7:je||ct(n,t),Pt(e,t,n);break;default:Pt(e,t,n)}}function sm(e,t){if(t.memoizedState===null&&(e=t.alternate,e!==null&&(e=e.memoizedState,e!==null))){e=e.dehydrated;try{hi(e)}catch(n){Ce(t,t.return,n)}}}function om(e,t){if(t.memoizedState===null&&(e=t.alternate,e!==null&&(e=e.memoizedState,e!==null&&(e=e.dehydrated,e!==null))))try{hi(e)}catch(n){Ce(t,t.return,n)}}function Q0(e){switch(e.tag){case 31:case 13:case 19:var t=e.stateNode;return t===null&&(t=e.stateNode=new nm),t;case 22:return e=e.stateNode,t=e._retryCache,t===null&&(t=e._retryCache=new nm),t;default:throw Error(d(435,e.tag))}}function Sr(e,t){var n=Q0(e);t.forEach(function(a){if(!n.has(a)){n.add(a);var i=av.bind(null,e,a);a.then(i,i)}})}function vt(e,t,n){var a=t.deletions;if(a!==null)for(var i=0;i<a.length;i++){var l=a[i],s=e,u=t,_=u;e:for(;_!==null;){switch(_.tag){case 27:if(Wn(_.type)){Ue=_.stateNode,kt=!1;break e}break;case 5:Ue=_.stateNode,kt=!1;break e;case 3:case 4:Ue=_.stateNode.containerInfo,kt=!0;break e}_=_.return}if(Ue===null)throw Error(d(160));rm(s,u,l),Ue=null,kt=!1,s=l.alternate,s!==null&&(s.return=null),l.return=null}if(t.subtreeFlags&13886)for(t=t.child;t!==null;)cm(t,e,n),t=t.sibling}var Ft=null;function cm(e,t,n){var a=e.alternate,i=e.flags;switch(e.tag){case 0:case 11:case 14:case 15:if(i&4&&(a=e.updateQueue,a=a!==null?a.events:null,a!==null))for(var l=0;l<a.length;l++){var s=a[l];s.ref.impl=s.nextImpl}vt(t,e,n),ht(e),i&4&&(Qn(3,e,e.return),Wi(3,e),Qn(5,e,e.return));break;case 1:vt(t,e,n),ht(e),i&512&&(je||a===null||ct(a,a.return)),i&64&&nt&&(e=e.updateQueue,e!==null&&(t=e.callbacks,t!==null&&(n=e.shared.hiddenCallbacks,e.shared.hiddenCallbacks=n===null?t:n.concat(t))));break;case 26:if(l=Ft,vt(t,e,n),ht(e),i&512&&(je||a===null||ct(a,a.return)),i&4)if(i=a!==null?a.memoizedState:null,n=e.memoizedState,a===null)if(n===null)if(e.stateNode===null)if(nt)e.stateNode=Km(e.type,e.memoizedProps,t.containerInfo,e);else{e:{t=e.type,n=e.memoizedProps,i=l.ownerDocument||l;t:switch(t){case"title":a=i.getElementsByTagName("title")[0],(!a||a[Ei]||a[lt]||a.namespaceURI==="http://www.w3.org/2000/svg"||a.hasAttribute("itemprop"))&&(a=i.createElement(t),i.head.insertBefore(a,i.querySelector("head > title"))),ut(a,t,n),a[lt]=e,et(a),t=a;break e;case"link":if(l=hf("link","href",i).get(t+(n.href||""))){for(s=0;s<l.length;s++)if(a=l[s],a.getAttribute("href")===(n.href==null||n.href===""?null:n.href)&&a.getAttribute("rel")===(n.rel==null?null:n.rel)&&a.getAttribute("title")===(n.title==null?null:n.title)&&a.getAttribute("crossorigin")===(n.crossOrigin==null?null:n.crossOrigin)){l.splice(s,1);break t}}a=i.createElement(t),ut(a,t,n),i.head.appendChild(a);break;case"meta":if(l=hf("meta","content",i).get(t+(n.content||""))){for(s=0;s<l.length;s++)if(a=l[s],a.getAttribute("content")===(n.content==null?null:""+n.content)&&a.getAttribute("name")===(n.name==null?null:n.name)&&a.getAttribute("property")===(n.property==null?null:n.property)&&a.getAttribute("http-equiv")===(n.httpEquiv==null?null:n.httpEquiv)&&a.getAttribute("charset")===(n.charSet==null?null:n.charSet)){l.splice(s,1);break t}}a=i.createElement(t),ut(a,t,n),i.head.appendChild(a);break;default:throw Error(d(468,t))}a[lt]=e,et(a),t=a}e.stateNode=t}else nt||qc(l,e.type,e.stateNode);else e.stateNode=vf(l,n,e.memoizedProps);else i!==n?(i===null?(t=a.stateNode,t===null||je||t.parentNode.removeChild(t)):i.count--,n===null?nt||qc(l,e.type,e.stateNode):vf(l,n,e.memoizedProps)):n===null&&e.stateNode!==null&&Uo(e,e.memoizedProps,a.memoizedProps);break;case 27:vt(t,e,n),ht(e),i&512&&(je||a===null||ct(a,a.return)),a!==null&&i&4&&Uo(e,e.memoizedProps,a.memoizedProps);break;case 5:if(l=on,on=!1,vt(t,e,n),on=l,ht(e),i&512&&(je||a===null||ct(a,a.return)),e.flags&32){t=e.stateNode;try{za(t,""),Ne=!0}catch(w){Ce(e,e.return,w)}}i&4&&e.stateNode!=null&&(t=e.memoizedProps,Uo(e,t,a!==null?a.memoizedProps:t)),i&1024&&(Qo=!0);break;case 6:if(vt(t,e,n),ht(e),i&4){if(e.stateNode===null)throw Error(d(162));t=e.memoizedProps,n=e.stateNode;try{n.nodeValue=t,Ne=!0}catch(w){Ce(e,e.return,w)}}break;case 3:if(Ne=!1,Lr=null,l=Ft,Ft=cl(t.containerInfo),vt(t,e,n),Ft=l,ht(e),i&4&&a!==null&&a.memoizedState.isDehydrated)try{hi(t.containerInfo)}catch(w){Ce(e,e.return,w)}Qo&&(Qo=!1,um(e)),Ne=!1;break;case 4:i=on,on=nt,a=Ou(),l=Ft,Ft=cl(e.stateNode.containerInfo),vt(t,e,n),ht(e),Ft=l,Ne&&el&&(kr=!0),Ne=a,on=i;break;case 12:vt(t,e,n),ht(e);break;case 31:vt(t,e,n),ht(e),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Sr(e,t)));break;case 13:vt(t,e,n),ht(e),e.child.flags&8192&&e.memoizedState!==null!=(a!==null&&a.memoizedState!==null)&&(Tr=Et()),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Sr(e,t)));break;case 22:l=e.memoizedState!==null,s=a!==null&&a.memoizedState!==null;var u=nt,_=je,g=on;nt=u||l,on=g||l,je=_||s,vt(t,e,n),je=_,on=g,nt=u,ht(e),i&8192&&(t=e.stateNode,t._visibility=l?t._visibility&-2:t._visibility|1,!l||a===null||s||nt||je||(t=s||je,n=nt,a=je,nt=l||nt,je=t,Kn(e,2),nt=n,je=a),!l&&on||Xo(e,l)),i&4&&(t=e.updateQueue,t!==null&&(n=t.retryQueue,n!==null&&(t.retryQueue=null,Sr(e,n))));break;case 19:vt(t,e,n),ht(e),i&4&&(t=e.updateQueue,t!==null&&(e.updateQueue=null,Sr(e,t)));break;case 30:i&512&&(je||a===null||ct(a,a.return)),i=Ou(),l=el,s=(n&335544064)===n,u=e.memoizedProps,el=s&&yn(u.default,u.update)!=="none",vt(t,e,n),ht(e),s&&a!==null&&Ne&&(e.flags|=4),el=l,Ne=i;break;case 21:break;case 7:i&512&&(je||a===null||ct(a,a.return)),a&&a.stateNode!==null&&(a.stateNode._fragmentFiber=e);default:vt(t,e,n),ht(e)}}function ht(e){var t=e.flags;if(t&2){try{for(var n,a=e.return;a!==null;){if(Z_(a)){n=a;break}a=a.return}a=null;for(var i=e.return;i!==null;){if(Do(i)){var l=i.stateNode;a===null?a=[l]:a.push(l)}if(zo(i))break;i=i.return}var s=a;if(n==null)throw Error(d(160));switch(n.tag){case 27:var u=n.stateNode,_=Bo(e);gr(e,_,u,s);break;case 5:var g=n.stateNode;n.flags&32&&(za(g,""),n.flags&=-33);var w=Bo(e);gr(e,w,g,s);break;case 3:case 4:var j=n.stateNode.containerInfo,v=Bo(e);Vo(e,v,j,s);break;default:throw Error(d(161))}}catch(b){Ce(e,e.return,b)}e.flags&=-3}t&4096&&(e.flags&=-4097)}function um(e){if(e.subtreeFlags&1024)for(e=e.child;e!==null;){var t=e;um(t),t.tag===5&&t.flags&1024&&(t=t.stateNode,vi=!0,t.reset(),vi=!1),e=e.sibling}}function $a(e,t){if(t.subtreeFlags&9270)for(t=t.child;t!==null;)dm(t,e),t=t.sibling;else tm(t)}function dm(e,t){var n=e.alternate;if(n===null)Lo(e,!1);else switch(e.tag){case 3:if(Ko=cn=!1,J_(),$a(t,e),!cn&&!kr){if(e=rn,e!==null)for(var a=0;a<e.length;a+=3){n=e[a];var i=e[a+1];Wm(n,e[a+2]),n=n.ownerDocument.documentElement,n!==null&&n.animate({opacity:[0,0],pointerEvents:["none","none"]},{duration:0,fill:"forwards",pseudoElement:"::view-transition-group("+i+")"})}e=t.containerInfo,e=e.nodeType===9?e.documentElement:e.ownerDocument.documentElement,e!==null&&e.style.viewTransitionName===""&&(e.style.viewTransitionName="none",e.animate({opacity:[0,0],pointerEvents:["none","none"]},{duration:0,fill:"forwards",pseudoElement:"::view-transition-group(root)"}),e.animate({width:[0,0],height:[0,0]},{duration:0,fill:"forwards",pseudoElement:"::view-transition"})),Ko=!0}rn=null;break;case 5:$a(t,e);break;case 4:a=cn,cn=!1,$a(t,e),cn&&(kr=!0),cn=a;break;case 22:e.memoizedState===null&&(n.memoizedState!==null?Lo(e,!1):$a(t,e));break;case 30:a=cn,i=J_(),cn=!1,$a(t,e),cn&&(e.flags|=4);var l=e.memoizedProps,s=e.stateNode;t=hn(l,s),s=hn(n.memoizedProps,s);var u=yn(l.default,l.update);u==="none"?t=!1:(l=n.memoizedState,n.memoizedState=null,n=e.child,wt=0,t=Io(e,n,t,s,u,l,!0),wt!==(l===null?0:l.length)&&(e.flags|=32)),(e.flags&4)!==0&&t?(ri(e,e.memoizedProps.onUpdate),rn=i):i!==null&&(i.push.apply(i,rn),rn=i),cn=(e.flags&32)!==0?!0:a;break;default:$a(t,e)}}function un(e,t){if(t.subtreeFlags&8772)for(t=t.child;t!==null;)am(e,t.alternate,t),t=t.sibling}function Kn(e,t){for(e=e.child;e!==null;){var n=e,a=t;switch(n.tag){case 0:case 11:case 14:case 15:Qn(4,n,n.return),Kn(n,a);break;case 1:ct(n,n.return);var i=n.stateNode;typeof i.componentWillUnmount=="function"&&K_(n,n.return,i),Kn(n,a);break;case 27:(a&2)!==0&&df(n.stateNode,n.type,n.memoizedProps);case 5:ct(n,n.return),n.tag!==5&&n.tag!==27||$i(n),Kn(n,a);break;case 6:$i(n);break;case 26:ct(n,n.return),i=n.stateNode,n.memoizedState!==null||i===null||je||i.parentNode.removeChild(i),Kn(n,a);break;case 22:n.memoizedState===null&&Kn(n,a);break;case 30:ct(n,n.return),Kn(n,a);break;case 7:ct(n,n.return);default:Kn(n,a)}e=e.sibling}}function Jt(e,t,n){for(n=(t.subtreeFlags&8772)!==0?n:n&-2,t=t.child;t!==null;){var a=t.alternate,i=e,l=t,s=l.flags,u=(n&1)!==0;switch(l.tag){case 0:case 11:case 15:Jt(i,l,n),Wi(4,l);break;case 1:if(Jt(i,l,n),a=l,i=a.stateNode,typeof i.componentDidMount=="function")try{i.componentDidMount()}catch(w){Ce(a,a.return,w)}if(a=l,i=a.updateQueue,i!==null){var _=a.stateNode;try{var g=i.shared.hiddenCallbacks;if(g!==null)for(i.shared.hiddenCallbacks=null,i=0;i<g.length;i++)Rd(g[i],_)}catch(w){Ce(a,a.return,w)}}u&&s&64&&Q_(l),ln(l,l.return);break;case 27:(n&2)!==0&&P_(l);case 5:l.tag!==5&&l.tag!==27||X_(l),Jt(i,l,n),u&&a===null&&s&4&&Mo(l),ln(l,l.return);break;case 6:X_(l);break;case 26:_=l.stateNode,l.memoizedState!==null||_===null||nt||qc(cl(_.ownerDocument),l.type,_),Jt(i,l,n),u&&a===null&&s&4&&Mo(l),ln(l,l.return);break;case 12:Jt(i,l,n);break;case 31:Jt(i,l,n),u&&s&4&&sm(i,l);break;case 13:Jt(i,l,n),u&&s&4&&om(i,l);break;case 22:l.memoizedState===null&&Jt(i,l,n),ln(l,l.return);break;case 30:Jt(i,l,n),ln(l,l.return);break;case 7:ln(l,l.return);default:Jt(i,l,n)}t=t.sibling}}function Po(e,t){var n=null;e!==null&&e.memoizedState!==null&&e.memoizedState.cachePool!==null&&(n=e.memoizedState.cachePool.pool),e=null,t.memoizedState!==null&&t.memoizedState.cachePool!==null&&(e=t.memoizedState.cachePool.pool),e!==n&&(e!=null&&e.refCount++,n!=null&&Vi(n))}function Fo(e,t){e=null,t.alternate!==null&&(e=t.alternate.memoizedState.cache),t=t.memoizedState.cache,t!==e&&(t.refCount++,e!=null&&Vi(e))}function It(e,t,n,a){var i=(n&335544064)===n;if(t.subtreeFlags&(i?10262:10256))for(t=t.child;t!==null;)_m(e,t,n,a),t=t.sibling;else i&&em(t)}function _m(e,t,n,a){var i=(n&335544064)===n;i&&t.alternate===null&&t.return!==null&&t.return.alternate!==null&&wr(t);var l=t.flags;switch(t.tag){case 0:case 11:case 15:It(e,t,n,a),l&2048&&Wi(9,t);break;case 1:It(e,t,n,a);break;case 3:It(e,t,n,a),i&&Ko&&(e=e.containerInfo,e=e.nodeType===9?e.body:e.nodeName==="HTML"?e.ownerDocument.body:e,e.style.viewTransitionName==="root"&&(e.style.viewTransitionName=""),e=e.ownerDocument.documentElement,e!==null&&e.style.viewTransitionName==="none"&&(e.style.viewTransitionName="")),l&2048&&(l=null,t.alternate!==null&&(l=t.alternate.memoizedState.cache),t=t.memoizedState.cache,t!==l&&(t.refCount++,l!=null&&Vi(l)));break;case 12:if(l&2048){It(e,t,n,a),l=t.stateNode;try{var s=t.memoizedProps,u=s.id,_=s.onPostCommit;typeof _=="function"&&_(u,t.alternate===null?"mount":"update",l.passiveEffectDuration,-0)}catch(g){Ce(t,t.return,g)}}else It(e,t,n,a);break;case 31:It(e,t,n,a);break;case 13:It(e,t,n,a);break;case 23:break;case 22:s=t.stateNode,u=t.alternate,t.memoizedState!==null?(i&&u!==null&&u.memoizedState===null&&wr(u),s._visibility&2?It(e,t,n,a):tl(e,t)):(i&&u!==null&&u.memoizedState!==null&&wr(t),s._visibility&2?It(e,t,n,a):(s._visibility|=2,ei(e,t,n,a,(t.subtreeFlags&10256)!==0||!1))),l&2048&&Po(u,t);break;case 24:It(e,t,n,a),l&2048&&Fo(t.alternate,t);break;case 30:i&&(l=t.alternate,l!==null&&(sn(l.child,!0),sn(t.child,!0))),It(e,t,n,a);break;default:It(e,t,n,a)}}function ei(e,t,n,a,i){for(i=i&&((t.subtreeFlags&10256)!==0||!1),t=t.child;t!==null;){var l=e,s=t,u=n,_=a,g=s.flags;switch(s.tag){case 0:case 11:case 15:ei(l,s,u,_,i),Wi(8,s);break;case 23:break;case 22:var w=s.stateNode;s.memoizedState!==null?w._visibility&2?ei(l,s,u,_,i):tl(l,s):(w._visibility|=2,ei(l,s,u,_,i)),i&&g&2048&&Po(s.alternate,s);break;case 24:ei(l,s,u,_,i),i&&g&2048&&Fo(s.alternate,s);break;default:ei(l,s,u,_,i)}t=t.sibling}}function tl(e,t){if(t.subtreeFlags&10256)for(t=t.child;t!==null;){var n=e,a=t,i=a.flags;switch(a.tag){case 22:tl(n,a),i&2048&&Po(a.alternate,a);break;case 24:tl(n,a),i&2048&&Fo(a.alternate,a);break;default:tl(n,a)}t=t.sibling}}var wa=8192;function ka(e,t,n){if(e.subtreeFlags&wa)for(e=e.child;e!==null;)mm(e,t,n),e=e.sibling}function mm(e,t,n){switch(e.tag){case 26:ka(e,t,n),e.flags&wa&&(e.memoizedState!==null?Jv(n,Ft,e.memoizedState,e.memoizedProps):(e=e.stateNode,(t&335544128)===t&&bf(n,e)));break;case 5:ka(e,t,n),e.flags&wa&&(e=e.stateNode,(t&335544128)===t&&bf(n,e));break;case 3:case 4:var a=Ft;Ft=cl(e.stateNode.containerInfo),ka(e,t,n),Ft=a;break;case 22:e.memoizedState===null&&(a=e.alternate,a!==null&&a.memoizedState!==null?(a=wa,wa=16777216,ka(e,t,n),wa=a):ka(e,t,n));break;case 30:if((e.flags&wa)!==0&&(a=e.memoizedProps.name,a!=null&&a!=="auto")){var i=e.stateNode;i.paired=null,qt===null&&(qt=new Map),qt.set(a,i)}ka(e,t,n);break;default:ka(e,t,n)}}function fm(e){var t=e.alternate;if(t!==null&&(e=t.child,e!==null)){t.child=null;do t=e.sibling,e.sibling=null,e=t;while(e!==null)}}function nl(e){var t=e.deletions;if((e.flags&16)!==0){if(t!==null)for(var n=0;n<t.length;n++){var a=t[n];at=a,vm(a,e)}fm(e)}if(e.subtreeFlags&10256)for(e=e.child;e!==null;)pm(e),e=e.sibling}function pm(e){switch(e.tag){case 0:case 11:case 15:nl(e),e.flags&2048&&Qn(9,e,e.return);break;case 3:nl(e);break;case 12:nl(e);break;case 22:var t=e.stateNode;e.memoizedState!==null&&t._visibility&2&&(e.return===null||e.return.tag!==13)?(t._visibility&=-3,Er(e)):nl(e);break;default:nl(e)}}function Er(e){var t=e.deletions;if((e.flags&16)!==0){if(t!==null)for(var n=0;n<t.length;n++){var a=t[n];at=a,vm(a,e)}fm(e)}for(e=e.child;e!==null;){switch(t=e,t.tag){case 0:case 11:case 15:Qn(8,t,t.return),Er(t);break;case 22:n=t.stateNode,n._visibility&2&&(n._visibility&=-3,Er(t));break;default:Er(t)}e=e.sibling}}function vm(e,t){for(;at!==null;){var n=at;switch(n.tag){case 0:case 11:case 15:Qn(8,n,t);break;case 23:case 22:if(n.memoizedState!==null&&n.memoizedState.cachePool!==null){var a=n.memoizedState.cachePool.pool;a!=null&&a.refCount++}break;case 24:Vi(n.memoizedState.cache)}if(a=n.child,a!==null)a.return=n,at=a;else e:for(n=e;at!==null;){a=at;var i=a.sibling,l=a.return;if(lm(a),a===n){at=null;break e}if(i!==null){i.return=l,at=i;break e}at=l}}}var K0={getCacheForType:function(e){var t=rt(Ke),n=t.data.get(e);return n===void 0&&(n=e(),t.data.set(e,n)),n},cacheSignal:function(){return rt(Ke).controller.signal}},X0=typeof WeakMap=="function"?WeakMap:Map,Ee=0,qe=null,ue=null,fe=0,Te=0,Rt=null,Xn=!1,ti=!1,Jo=!1,jn=0,Ie=0,Zn=0,Na=0,jr=0,zt=0,ni=0,al=null,Nt=null,Wo=!1,Tr=0,hm=0,Cr=1/0,Or=null,Pn=null,He=0,Wt=null,Sa=null,dn=0,$o=0,ec=null,ym=null,ai=null,ii=null,li=null,il=0,Ar=null;function Dt(){return(Ee&2)!==0&&fe!==0?fe&-fe:R.T!==null?uc():bu()}function gm(){if(zt===0)if((fe&536870912)===0||oe){var e=Sl;Sl<<=1,(Sl&3932160)===0&&(Sl=262144),zt=e}else zt=536870912;return e=st.current,e!==null&&(e.flags|=32),zt}function ri(e,t){if(t!=null){var n=e.stateNode,a=n.ref;a===null&&(a=n.ref=$m(hn(e.memoizedProps,n))),ii===null&&(ii=[]),ii.push(t.bind(null,a))}}function St(e,t,n){(e===qe&&(Te===2||Te===9)||e.cancelPendingCommit!==null)&&(si(e,0),Fn(e,fe,zt,!1)),Si(e,n),((Ee&2)===0||e!==qe)&&(e===qe&&((Ee&2)===0&&(Na|=n),Ie===4&&Fn(e,fe,zt,!1)),_n(e))}function xm(e,t,n){if((Ee&6)!==0)throw Error(d(327));var a=!n&&(t&127)===0&&(t&e.expiredLanes)===0||Ni(e,t),i=a?F0(e,t):nc(e,t,!0),l=a;do{if(i===0){ti&&!a&&Fn(e,t,0,!1);break}else{if(n=e.current.alternate,l&&!Z0(n)){i=nc(e,t,!1),l=!1;continue}if(i===2){if(l=t,e.errorRecoveryDisabledLanes&l)var s=0;else s=e.pendingLanes&-536870913,s=s!==0?s:s&536870912?536870912:0;if(s!==0){t=s;e:{var u=e;i=al;var _=u.current.memoizedState.isDehydrated;if(_&&(si(u,s).flags|=256),s=nc(u,s,!1),s!==2&&s!==6){if(Jo&&!_){u.errorRecoveryDisabledLanes|=l,Na|=l,i=4;break e}l=Nt,Nt=i,l!==null&&(Nt===null?Nt=l:Nt.push.apply(Nt,l))}i=s}if(l=!1,i!==2)continue}}if(i===1){si(e,0),Fn(e,t,0,!0);break}e:{switch(a=e,l=i,l){case 0:case 1:throw Error(d(345));case 4:if((t&4194048)!==t&&(t&62914560)!==t)break;case 6:Fn(a,t,zt,!Xn);break e;case 2:Nt=null;break;case 3:case 5:break;default:throw Error(d(329))}if((t&62914560)===t&&(i=Tr+300-Et(),10<i)){if(Fn(a,t,zt,!Xn),jl(a,0,!0)!==0)break e;dn=t,a.timeoutHandle=bc(bm.bind(null,a,n,Nt,Or,Wo,t,zt,Na,ni,Xn,l,"Throttled",-0,0),i);break e}bm(a,n,Nt,Or,Wo,t,zt,Na,ni,Xn,l,null,-0,0)}}break}while(!0);_n(e)}function bm(e,t,n,a,i,l,s,u,_,g,w,j,v,b){e.timeoutHandle=-1;var B=t.subtreeFlags,X=(l&335544064)===l;if(j=null,(X||B&8192||(B&16785408)===16785408)&&(j={stylesheets:null,count:0,imgCount:0,imgBytes:0,suspenseyImages:[],waitingForImages:!0,waitingForViewTransition:!1,unsuspend:tn},qt=null,mm(t,l,j),X&&(B=j,X=e.containerInfo,X=(X.nodeType===9?X:X.ownerDocument).__reactViewTransition,X!=null&&(B.count++,B.waitingForViewTransition=!0,B=_l.bind(B),X.finished.then(B,B))),B=(l&62914560)===l?Tr-Et():(l&4194048)===l?hm-Et():0,B=Wv(j,B),B!==null)){dn=l,e.cancelPendingCommit=B(Cm.bind(null,e,t,l,n,a,i,s,u,_,g,w,j,null,v,b)),Fn(e,l,s,!g);return}Cm(e,t,l,n,a,i,s,u,_,g,w,j)}function Z0(e){for(var t=e;;){var n=t.tag;if((n===0||n===11||n===15)&&t.flags&16384&&(n=t.updateQueue,n!==null&&(n=n.stores,n!==null)))for(var a=0;a<n.length;a++){var i=n[a],l=i.getSnapshot;i=i.value;try{if(!Ot(l(),i))return!1}catch{return!1}}if(n=t.child,t.subtreeFlags&16384&&n!==null)n.return=t,t=n;else{if(t===e)break;for(;t.sibling===null;){if(t.return===null||t.return===e)return!0;t=t.return}t.sibling.return=t.return,t=t.sibling}}return!0}function Fn(e,t,n,a){t=vu(e,t),t&=~jr,t&=~Na,e.suspendedLanes|=t,e.pingedLanes&=~t,a&&(e.warmLanes|=t),a=e.expirationTimes;for(var i=t;0<i;){var l=31-Tt(i),s=1<<l;a[l]=-1,i&=~s}n!==0&&yu(e,n,t)}function qr(){return(Ee&6)===0?(ll(0),!1):!0}function tc(){if(ue!==null){if(Te===0)var e=ue.return;else e=ue,bn=_a=null,oo(e),Xa=null,Gi=0,e=ue;for(;e!==null;)I_(e.alternate,e),e=e.return;ue=null}}function si(e,t){var n=e.timeoutHandle;return n!==-1&&(e.timeoutHandle=-1,yv(n)),n=e.cancelPendingCommit,n!==null&&(e.cancelPendingCommit=null,n()),dn=0,tc(),qe=e,ue=n=gn(e.current,null),fe=t,Te=0,Rt=null,Xn=!1,ti=Ni(e,t),Jo=!1,ni=zt=jr=Na=Zn=Ie=0,Nt=al=null,Wo=!1,jn=vu(e,t),Ll(),n}function wm(e,t){re=null,R.H=dr,t===Ka||t===Jl?(t=Cd(),Te=3):t===Ps?(t=Cd(),Te=4):Te=t===No?8:t!==null&&typeof t=="object"&&typeof t.then=="function"?6:1,Rt=t,ue===null&&(Ie=1,_r(e,Lt(t,e.current)))}function km(){var e=st.current;return e===null?!0:(fe&4194048)===fe?mt===null:(fe&62914560)===fe||(fe&536870912)!==0?e===mt:!1}function Nm(){var e=R.H;return R.H=dr,e===null?dr:e}function Sm(){var e=R.A;return R.A=K0,e}function Rr(){Ie=4,Xn||(fe&4194048)!==fe&&st.current!==null||(ti=!0),(Zn&134217727)===0&&(Na&134217727)===0||qe===null||Fn(qe,fe,zt,!1)}function nc(e,t,n){var a=Ee;Ee|=2;var i=Nm(),l=Sm();(qe!==e||fe!==t)&&(Or=null,si(e,t)),t=!1;var s=Ie;e:do try{if(Te!==0&&ue!==null){var u=ue,_=Rt;switch(Te){case 8:tc(),s=6;break e;case 3:case 2:case 9:case 6:st.current===null&&(t=!0);var g=Te;if(Te=0,Rt=null,oi(e,u,_,g),n&&ti){s=0;break e}break;default:g=Te,Te=0,Rt=null,oi(e,u,_,g)}}P0(),s=Ie;break}catch(w){wm(e,w)}while(!0);return t&&e.shellSuspendCounter++,bn=_a=null,Ee=a,R.H=i,R.A=l,ue===null&&(qe=null,fe=0,Ll()),s}function P0(){for(;ue!==null;)Em(ue)}function F0(e,t){var n=Ee;Ee|=2;var a=Nm(),i=Sm();qe!==e||fe!==t?(Or=null,Cr=Et()+500,si(e,t)):ti=Ni(e,t);e:do try{if(Te!==0&&ue!==null){t=ue;var l=Rt;t:switch(Te){case 1:Te=0,Rt=null,oi(e,t,l,1);break;case 2:case 9:if(jd(l)){Te=0,Rt=null,jm(t);break}t=function(){Te!==2&&Te!==9||qe!==e||(Te=7),_n(e)},l.then(t,t);break e;case 3:Te=7;break e;case 4:Te=5;break e;case 7:jd(l)?(Te=0,Rt=null,jm(t)):(Te=0,Rt=null,oi(e,t,l,7));break;case 5:var s=null;switch(ue.tag){case 26:s=ue.memoizedState;case 5:case 27:var u=ue;if(s?gf(s):u.stateNode.complete){Te=0,Rt=null;var _=u.sibling;if(_!==null)ue=_;else{var g=u.return;g!==null?(ue=g,zr(g)):ue=null}break t}}Te=0,Rt=null,oi(e,t,l,5);break;case 6:Te=0,Rt=null,oi(e,t,l,6);break;case 8:tc(),Ie=6;break e;default:throw Error(d(462))}}J0();break}catch(w){wm(e,w)}while(!0);return bn=_a=null,R.H=a,R.A=i,Ee=n,ue!==null?0:(qe=null,fe=0,Ll(),Ie)}function J0(){for(;ue!==null&&!fp();)Em(ue)}function Em(e){var t=G_(e.alternate,e,jn);e.memoizedProps=e.pendingProps,t===null?zr(e):ue=t}function jm(e){var t=e,n=t.alternate;switch(t.tag){case 15:case 0:t=D_(n,t,t.pendingProps,t.type,void 0,fe);break;case 11:t=D_(n,t,t.pendingProps,t.type.render,t.ref,fe);break;case 5:oo(t);var a=t;a===tt&&(oe?(Kl(a),a.tag===5&&a.stateNode!=null&&(Re=a.stateNode)):(Kl(a),oe=!0));default:I_(n,t),t=ue=vd(t,jn),t=G_(n,t,jn)}e.memoizedProps=e.pendingProps,t===null?zr(e):ue=t}function oi(e,t,n,a){bn=_a=null,oo(t),Xa=null,Gi=0;var i=t.return;try{if(B0(e,i,t,n,fe)){Ie=1,_r(e,Lt(n,e.current)),ue=null;return}}catch(l){if(i!==null)throw ue=i,l;Ie=1,_r(e,Lt(n,e.current)),ue=null;return}t.flags&32768?(oe||a===1?e=!0:ti||(fe&536870912)!==0?e=!1:(Xn=e=!0,(a===2||a===9||a===3||a===6)&&(a=st.current,a!==null&&a.tag===13&&(a.flags|=16384))),Tm(t,e)):zr(t)}function zr(e){var t=e;do{if((t.flags&32768)!==0){Tm(t,Xn);return}e=t.return;var n=G0(t.alternate,t,jn);if(n!==null){ue=n;return}if(t=t.sibling,t!==null){ue=t;return}ue=t=e}while(t!==null);Ie===0&&(Ie=5)}function Tm(e,t){do{var n=Y0(e.alternate,e);if(n!==null){n.flags&=32767,ue=n;return}if(n=e.return,n!==null&&(n.flags|=32768,n.subtreeFlags=0,n.deletions=null),!t&&(e=e.sibling,e!==null)){ue=e;return}ue=e=n}while(e!==null);Ie=6,ue=null}function Cm(e,t,n,a,i,l,s,u,_,g,w,j){e.cancelPendingCommit=null;do Dr();while(He!==0);if((Ee&6)!==0)throw Error(d(327));if(t!==null){if(t===e.current)throw Error(d(177));e===qe&&(ue=qe=null,fe=0),Sa=t,Wt=e,dn=n,ec=i,ym=a,W0(e,t,n,s,u,_,j)}}function W0(e,t,n,a,i,l,s){var u=t.lanes|t.childLanes;if($o=u,u|=Ms,Np(e,n,u,a,i,l),ii=null,(n&335544064)===n?(li=E0(e),a=10262):(li=null,a=10256),(t.subtreeFlags&a)!==0||(t.flags&a)!==0?(e.callbackNode=null,e.callbackPriority=0,iv(kl,function(){return rc(),null})):(e.callbackNode=null,e.callbackPriority=0),xr=!1,a=(t.flags&13878)!==0,(t.subtreeFlags&13878)!==0||a){a=R.T,R.T=null,i=N.p,N.p=2,l=Ee,Ee|=4;try{I0(e,t,n)}finally{Ee=l,N.p=i,R.T=a}}He=1,xr?ai=Nv(s,e.containerInfo,li,ac,ic,ev,lc,rc,$0):(ac(),ic(),lc())}function $0(e){if(He!==0){var t=Wt.onRecoverableError;t(e,{componentStack:null})}}function ev(){He===3&&(He=0,dm(Sa,Wt),He=4)}function ac(){if(He===1){He=0;var e=Wt,t=Sa,n=dn,a=(t.flags&13878)!==0;if((t.subtreeFlags&13878)!==0||a){a=R.T,R.T=null;var i=N.p;N.p=2;var l=Ee;Ee|=4;try{el=kr=!1,cm(t,e,n),n=yc;var s=rd(e.containerInfo),u=n.focusedElem,_=n.selectionRange;if(s!==u&&u&&u.ownerDocument&&ld(u.ownerDocument.documentElement,u)){if(_!==null&&As(u)){var g=_.start,w=_.end;if(w===void 0&&(w=g),"selectionStart"in u)u.selectionStart=g,u.selectionEnd=Math.min(w,u.value.length);else{var j=u.ownerDocument||document,v=j&&j.defaultView||window;if(v.getSelection){var b=v.getSelection(),B=u.textContent.length,X=Math.min(_.start,B),se=_.end===void 0?X:Math.min(_.end,B);!b.extend&&X>se&&(s=se,se=X,X=s);var h=id(u,X),f=id(u,se);if(h&&f&&(b.rangeCount!==1||b.anchorNode!==h.node||b.anchorOffset!==h.offset||b.focusNode!==f.node||b.focusOffset!==f.offset)){var x=j.createRange();x.setStart(h.node,h.offset),b.removeAllRanges(),X>se?(b.addRange(x),b.extend(f.node,f.offset)):(x.setEnd(f.node,f.offset),b.addRange(x))}}}}for(j=[],b=u;b=b.parentNode;)b.nodeType===1&&j.push({element:b,left:b.scrollLeft,top:b.scrollTop});for(typeof u.focus=="function"&&u.focus(),u=0;u<j.length;u++){var E=j[u];E.element.scrollLeft=E.left,E.element.scrollTop=E.top}}vi=!!hc,yc=hc=null}finally{Ee=l,N.p=i,R.T=a}}e.current=t,He=2}}function ic(){if(He===2){He=0;var e=Wt,t=Sa,n=(t.flags&8772)!==0;if((t.subtreeFlags&8772)!==0||n){n=R.T,R.T=null;var a=N.p;N.p=2;var i=Ee;Ee|=4;try{am(e,t.alternate,t)}finally{Ee=i,N.p=a,R.T=n}}He=3}}function lc(){if(He===4||He===3){He=0;var e=ai;ai=null,pp();var t=Wt,n=Sa,a=dn,i=ym,l=(a&335544064)===a?10262:10256;if((n.subtreeFlags&l)!==0||(n.flags&l)!==0?He=5:(He=0,Sa=Wt=null,Om(t,t.pendingLanes)),l=t.pendingLanes,l===0&&(Pn=null),ms(a),n=n.stateNode,jt&&typeof jt.onCommitFiberRoot=="function")try{jt.onCommitFiberRoot(ki,n,void 0,(n.current.flags&128)===128)}catch{}if(i!==null){n=R.T,l=N.p,N.p=2,R.T=null;try{for(var s=t.onRecoverableError,u=0;u<i.length;u++){var _=i[u];s(_.value,{componentStack:_.stack})}}finally{R.T=n,N.p=l}}if(i=ii,s=li,li=null,i!==null&&(ii=null,s===null&&(s=[]),e!==null))for(_=0;_<i.length;_++)n=(0,i[_])(s),n!==void 0&&e.finished.finally(n);(dn&3)!==0&&Dr(),_n(t),l=t.pendingLanes,(a&261930)!==0&&(l&42)!==0?t===Ar?il++:(il=0,Ar=t):(il=0,Ar=null),ll(0)}}function Om(e,t){(e.pooledCacheLanes&=t)===0&&(t=e.pooledCache,t!=null&&(e.pooledCache=null,Vi(t)))}function Dr(){return ai!==null&&(ai.skipTransition(),ai=null),ac(),ic(),lc(),rc()}function rc(){if(He!==5)return!1;var e=Wt,t=$o;$o=0;var n=ms(dn),a=R.T,i=N.p;try{N.p=32>n?32:n,R.T=null,n=ec,ec=null;var l=Wt,s=dn;if(He=0,Sa=Wt=null,dn=0,(Ee&6)!==0)throw Error(d(331));var u=Ee;if(Ee|=4,pm(l.current),_m(l,l.current,s,n),Ee=u,ll(0,!1),jt&&typeof jt.onPostCommitFiberRoot=="function")try{jt.onPostCommitFiberRoot(ki,l)}catch{}return!0}finally{N.p=i,R.T=a,Om(e,t)}}function Am(e,t,n){t=Lt(n,t),t=ko(e.stateNode,t,2),e=Hn(e,t,2),e!==null&&(Si(e,2),_n(e))}function Ce(e,t,n){if(e.tag===3)Am(e,e,n);else for(;t!==null;){if(t.tag===3){Am(t,e,n);break}else if(t.tag===1){var a=t.stateNode;if(typeof t.type.getDerivedStateFromError=="function"||typeof a.componentDidCatch=="function"&&(Pn===null||!Pn.has(a))){e=Lt(n,e),n=j_(2),a=Hn(t,n,2),a!==null&&(T_(n,a,t,e),Si(a,2),_n(a));break}}t=t.return}}function sc(e,t,n){var a=e.pingCache;if(a===null){a=e.pingCache=new X0;var i=new Set;a.set(t,i)}else i=a.get(t),i===void 0&&(i=new Set,a.set(t,i));i.has(n)||(Jo=!0,i.add(n),e=tv.bind(null,e,t,n),t.then(e,e))}function tv(e,t,n){var a=e.pingCache;a!==null&&a.delete(t),e.pingedLanes|=e.suspendedLanes&n,e.warmLanes&=~n,qe===e&&(fe&n)===n&&((Ie===4||Ie===3&&(fe&62914560)===fe&&300>Et()-Tr)&&(Ee&2)===0?si(e,0):jr|=n,ni===fe&&(ni=0)),_n(e)}function qm(e,t){t===0&&(t=hu()),e=ca(e,t),e!==null&&(Si(e,t),_n(e))}function nv(e){var t=e.memoizedState,n=0;t!==null&&(n=t.retryLane),qm(e,n)}function av(e,t){var n=0;switch(e.tag){case 31:case 13:var a=e.stateNode,i=e.memoizedState;i!==null&&(n=i.retryLane);break;case 19:a=e.stateNode;break;case 22:a=e.stateNode._retryCache;break;default:throw Error(d(314))}a!==null&&a.delete(t),qm(e,n)}function iv(e,t){return cs(e,t)}var ci=null,ui=null,oc=!1,Mr=!1,cc=!1,Jn=0;function _n(e){e!==ui&&e.next===null&&(ui===null?ci=ui=e:ui=ui.next=e),Mr=!0,oc||(oc=!0,rv())}function ll(e,t){if(!cc&&Mr){cc=!0;do for(var n=!1,a=ci;a!==null;){if(e!==0){var i=a.pendingLanes;if(i===0)var l=0;else{var s=a.suspendedLanes,u=a.pingedLanes;l=(1<<31-Tt(42|e)+1)-1,l&=i&~(s&~u),l=l&201326741?l&201326741|1:l?l|2:0}l!==0&&(n=!0,Mm(a,l))}else l=fe,l=jl(a,a===qe?l:0,a.cancelPendingCommit!==null||a.timeoutHandle!==-1),(l&3)===0||Ni(a,l)||(n=!0,Mm(a,l));a=a.next}while(n);cc=!1}}function lv(){Rm()}function Rm(){Mr=oc=!1;var e=0;Jn!==0&&hv()&&(e=Jn);for(var t=Et(),n=null,a=ci;a!==null;){var i=a.next,l=zm(a,t);l===0?(a.next=null,n===null?ci=i:n.next=i,i===null&&(ui=n)):(n=a,(e!==0||(l&3)!==0)&&(Mr=!0)),a=i}He!==0&&He!==5||ll(e),Jn!==0&&(Jn=0)}function zm(e,t){for(var n=e.suspendedLanes,a=e.pingedLanes,i=e.expirationTimes,l=e.pendingLanes&-62914561;0<l;){var s=31-Tt(l),u=1<<s,_=i[s];_===-1?((u&n)===0||(u&a)!==0)&&(i[s]=kp(u,t)):_<=t&&(e.expiredLanes|=u),l&=~u}if(t=qe,n=fe,n=jl(e,e===t?n:0,e.cancelPendingCommit!==null||e.timeoutHandle!==-1),a=e.callbackNode,n===0||e===t&&(Te===2||Te===9)||e.cancelPendingCommit!==null)return a!==null&&a!==null&&us(a),e.callbackNode=null,e.callbackPriority=0;if((n&3)===0||Ni(e,n)){if(t=n&-n,t===e.callbackPriority)return t;switch(a!==null&&us(a),ms(n)){case 2:case 8:n=fu;break;case 32:n=kl;break;case 268435456:n=pu;break;default:n=kl}return a=Dm.bind(null,e),n=cs(n,a),e.callbackPriority=t,e.callbackNode=n,t}return a!==null&&a!==null&&us(a),e.callbackPriority=2,e.callbackNode=null,2}function Dm(e,t){if(He!==0&&He!==5)return e.callbackNode=null,e.callbackPriority=0,null;var n=e.callbackNode;if(Dr()&&e.callbackNode!==n)return null;var a=fe;return a=jl(e,e===qe?a:0,e.cancelPendingCommit!==null||e.timeoutHandle!==-1),a===0?null:(xm(e,a,t),zm(e,Et()),e.callbackNode!=null&&e.callbackNode===n?Dm.bind(null,e):null)}function Mm(e,t){if(Dr())return null;xm(e,t,!0)}function rv(){gv(function(){(Ee&6)!==0?cs(mu,lv):Rm()})}function uc(){if(Jn===0){var e=pa;e===0&&(e=Nl,Nl<<=1,(Nl&261888)===0&&(Nl=256)),Jn=e}return Jn}function Um(e){return e==null||typeof e=="symbol"||typeof e=="boolean"?null:typeof e=="function"?e:ql(e)}function sv(e,t,n,a,i){if(t==="submit"&&n&&n.stateNode===i){var l=Um((i[xt]||null).action),s=a.submitter;s&&(t=(t=s[xt]||null)?Um(t.formAction):s.getAttribute("formAction"),t!==null&&(l=t,s=null));var u=new Ml("action","action",null,a,i);e.push({event:u,listeners:[{instance:null,listener:function(){if(a.defaultPrevented){if(Jn!==0){var _=new FormData(i,s);yo(n,{pending:!0,data:_,method:i.method,action:l},null,_)}}else typeof l=="function"&&(u.preventDefault(),_=new FormData(i,s),yo(n,{pending:!0,data:_,method:i.method,action:l},l,_))},currentTarget:i}]})}}for(var dc=0;dc<Ds.length;dc++){var _c=Ds[dc],ov=_c.toLowerCase(),cv=_c[0].toUpperCase()+_c.slice(1);Zt(ov,"on"+cv)}Zt(cd,"onAnimationEnd"),Zt(ud,"onAnimationIteration"),Zt(dd,"onAnimationStart"),Zt("dblclick","onDoubleClick"),Zt("focusin","onFocus"),Zt("focusout","onBlur"),Zt(y0,"onTransitionRun"),Zt(g0,"onTransitionStart"),Zt(x0,"onTransitionCancel"),Zt(_d,"onTransitionEnd"),qa("onMouseEnter",["mouseout","mouseover"]),qa("onMouseLeave",["mouseout","mouseover"]),qa("onPointerEnter",["pointerout","pointerover"]),qa("onPointerLeave",["pointerout","pointerover"]),ra("onChange","change click focusin focusout input keydown keyup selectionchange".split(" ")),ra("onSelect","focusout contextmenu dragend focusin keydown keyup mousedown mouseup selectionchange".split(" ")),ra("onBeforeInput",["compositionend","keypress","textInput","paste"]),ra("onCompositionEnd","compositionend focusout keydown keypress keyup mousedown".split(" ")),ra("onCompositionStart","compositionstart focusout keydown keypress keyup mousedown".split(" ")),ra("onCompositionUpdate","compositionupdate focusout keydown keypress keyup mousedown".split(" "));var rl="abort canplay canplaythrough durationchange emptied encrypted ended error loadeddata loadedmetadata loadstart pause play playing progress ratechange resize seeked seeking stalled suspend timeupdate volumechange waiting".split(" "),uv=new Set("beforetoggle cancel close invalid load scroll scrollend toggle".split(" ").concat(rl));function Bm(e,t){t=(t&4)!==0;for(var n=0;n<e.length;n++){var a=e[n],i=a.event;a=a.listeners;e:{var l=void 0;if(t)for(var s=a.length-1;0<=s;s--){var u=a[s],_=u.instance,g=u.currentTarget;if(u=u.listener,_!==l&&i.isPropagationStopped())break e;l=u,i.currentTarget=g;try{l(i)}catch(w){Vl(w)}i.currentTarget=null,l=_}else for(s=0;s<a.length;s++){if(u=a[s],_=u.instance,g=u.currentTarget,u=u.listener,_!==l&&i.isPropagationStopped())break e;l=u,i.currentTarget=g;try{l(i)}catch(w){Vl(w)}i.currentTarget=null,l=_}}}}function de(e,t){var n=t[ku];n===void 0&&(n=t[ku]=new Set);var a=e+"__bubble";n.has(a)||(Vm(t,e,2,!1),n.add(a))}function mc(e,t,n){var a=0;t&&(a|=4),Vm(n,e,a,t)}var Ur="_reactListening"+Math.random().toString(36).slice(2);function fc(e){if(!e[Ur]){e[Ur]=!0,Eu.forEach(function(n){n!=="selectionchange"&&(uv.has(n)||mc(n,!1,e),mc(n,!0,e))});var t=e.nodeType===9?e:e.ownerDocument;t===null||t[Ur]||(t[Ur]=!0,mc("selectionchange",!1,t))}}function Vm(e,t,n,a){switch(Cf(t)){case 2:var i=nh;break;case 8:i=ah;break;default:i=zc}n=i.bind(null,t,n,e),i=void 0,!bs||t!=="touchstart"&&t!=="touchmove"&&t!=="wheel"||(i=!0),a?i!==void 0?e.addEventListener(t,n,{capture:!0,passive:i}):e.addEventListener(t,n,!0):i!==void 0?e.addEventListener(t,n,{passive:i}):e.addEventListener(t,n,!1)}function pc(e,t,n,a,i){var l=a;if((t&1)===0&&(t&2)===0&&a!==null)e:for(;;){if(a===null)return;var s=a.tag;if(s===3||s===4){var u=a.stateNode.containerInfo;if(u===i)break;if(s===4)for(s=a.return;s!==null;){var _=s.tag;if((_===3||_===4)&&s.stateNode.containerInfo===i)return;s=s.return}for(;u!==null;){if(s=la(u),s===null)return;if(_=s.tag,_===5||_===6||_===26||_===27){a=l=s;continue e}u=u.parentNode}}a=a.return}Vu(function(){var g=l,w=gs(n),j=[];e:{var v=md.get(e);if(v!==void 0){var b=Ml,B=e;switch(e){case"keypress":if(zl(n)===0)break e;case"keydown":case"keyup":b=Zp;break;case"focusin":B="focus",b=Ss;break;case"focusout":B="blur",b=Ss;break;case"beforeblur":case"afterblur":b=Ss;break;case"click":if(n.button===2)break e;case"auxclick":case"dblclick":case"mousedown":case"mousemove":case"mouseup":case"mouseout":case"mouseover":case"contextmenu":b=Gu;break;case"drag":case"dragend":case"dragenter":case"dragexit":case"dragleave":case"dragover":case"dragstart":case"drop":b=Mp;break;case"touchcancel":case"touchend":case"touchmove":case"touchstart":b=$p;break;case cd:case ud:case dd:b=Vp;break;case _d:b=t0;break;case"scroll":case"scrollend":b=zp;break;case"wheel":b=a0;break;case"copy":case"cut":case"paste":b=Hp;break;case"gotpointercapture":case"lostpointercapture":case"pointercancel":case"pointerdown":case"pointermove":case"pointerout":case"pointerover":case"pointerup":b=Iu;break;case"submit":b=Jp;break;case"toggle":case"beforetoggle":b=l0}var X=(t&4)!==0,se=!X&&(e==="scroll"||e==="scrollend"),h=X?v!==null?v+"Capture":null:v;X=[];for(var f=g,x;f!==null;){var E=f;if(x=E.stateNode,E=E.tag,E!==5&&E!==26&&E!==27||x===null||h===null||(E=Ti(f,h),E!=null&&X.push(sl(f,E,x))),se)break;f=f.return}0<X.length&&(v=new b(v,B,null,n,w),j.push({event:v,listeners:X}))}}if((t&7)===0){e:{if(b=e==="mouseover"||e==="pointerover",v=e==="mouseout"||e==="pointerout",b&&n!==ys&&(B=n.relatedTarget||n.fromElement)&&(la(B)||B[Ca]))break e;(v||b)&&(B=w.window===w?w:(b=w.ownerDocument)?b.defaultView||b.parentWindow:window,v?(b=n.relatedTarget||n.toElement,v=g,b=b?la(b):null,b!==null&&(se=q(b),X=b.tag,b!==se||X!==5&&X!==27&&X!==6)&&(b=null)):(v=null,b=g),v!==b&&(X=Gu,E="onMouseLeave",h="onMouseEnter",f="mouse",(e==="pointerout"||e==="pointerover")&&(X=Iu,E="onPointerLeave",h="onPointerEnter",f="pointer"),se=v==null?B:ji(v),x=b==null?B:ji(b),B=new X(E,f+"leave",v,n,w),B.target=se,B.relatedTarget=x,E=null,la(w)===g&&(X=new X(h,f+"enter",b,n,w),X.target=x,X.relatedTarget=se,E=X),se=E,X=v&&b?V(v,b,dv):null,v!==null&&Lm(j,B,v,X,!1),b!==null&&se!==null&&Lm(j,se,b,X,!0)))}e:{if(v=g?ji(g):window,b=v.nodeName&&v.nodeName.toLowerCase(),b==="select"||b==="input"&&v.type==="file")var G=Wu;else if(Fu(v))if($u)G=p0;else{G=m0;var pe=_0}else b=v.nodeName,!b||b.toLowerCase()!=="input"||v.type!=="checkbox"&&v.type!=="radio"?g&&hs(g.elementType)&&(G=Wu):G=f0;if(G&&(G=G(e,g))){Ju(j,G,n,w);break e}pe&&pe(e,v,g)}switch(pe=g?ji(g):window,e){case"focusin":(Fu(pe)||pe.contentEditable==="true")&&(Ba=pe,qs=g,Mi=null);break;case"focusout":Mi=qs=Ba=null;break;case"mousedown":Rs=!0;break;case"contextmenu":case"mouseup":case"dragend":Rs=!1,sd(j,n,w);break;case"selectionchange":if(h0)break;case"keydown":case"keyup":sd(j,n,w)}var ee;if(js)e:{switch(e){case"compositionstart":var ie="onCompositionStart";break e;case"compositionend":ie="onCompositionEnd";break e;case"compositionupdate":ie="onCompositionUpdate";break e}ie=void 0}else Ua?Zu(e,n)&&(ie="onCompositionEnd"):e==="keydown"&&n.keyCode===229&&(ie="onCompositionStart");ie&&(Qu&&n.locale!=="ko"&&(Ua||ie!=="onCompositionStart"?ie==="onCompositionEnd"&&Ua&&(ee=Lu()):(qn=w,ws="value"in qn?qn.value:qn.textContent,Ua=!0)),pe=Br(g,ie),0<pe.length&&(ie=new Yu(ie,e,null,n,w),j.push({event:ie,listeners:pe}),ee?ie.data=ee:(ee=Pu(n),ee!==null&&(ie.data=ee)))),(ee=s0?o0(e,n):c0(e,n))&&(ie=Br(g,"onBeforeInput"),0<ie.length&&(pe=new Yu("onBeforeInput","beforeinput",null,n,w),j.push({event:pe,listeners:ie}),pe.data=ee)),sv(j,e,g,n,w)}Bm(j,t)})}function sl(e,t,n){return{instance:e,listener:t,currentTarget:n}}function Br(e,t){for(var n=t+"Capture",a=[];e!==null;){var i=e,l=i.stateNode;if(i=i.tag,i!==5&&i!==26&&i!==27||l===null||(i=Ti(e,n),i!=null&&a.unshift(sl(e,i,l)),i=Ti(e,t),i!=null&&a.push(sl(e,i,l))),e.tag===3)return a;e=e.return}return[]}function dv(e){if(e===null)return null;do e=e.return;while(e&&e.tag!==5&&e.tag!==27);return e||null}function Lm(e,t,n,a,i){for(var l=t._reactName,s=[];n!==null&&n!==a;){var u=n,_=u.alternate,g=u.stateNode;if(u=u.tag,_!==null&&_===a)break;u!==5&&u!==26&&u!==27||g===null||(_=g,i?(g=Ti(n,l),g!=null&&s.unshift(sl(n,g,_))):i||(g=Ti(n,l),g!=null&&s.push(sl(n,g,_)))),n=n.return}s.length!==0&&e.push({event:t,listeners:s})}var _v=/\r\n?/g,mv=/\u0000|\uFFFD/g;function Hm(e){return(typeof e=="string"?e:""+e).replace(_v,`
`).replace(mv,"")}function Gm(e,t){return t=Hm(t),Hm(e)===t}function Oe(e,t,n,a,i,l){switch(n){case"children":if(typeof a=="string")t==="body"||t==="textarea"&&a===""||za(e,a);else if(typeof a=="number"||typeof a=="bigint")t!=="body"&&za(e,""+a);else return;break;case"className":Al(e,"class",a);break;case"tabIndex":Al(e,"tabindex",a);break;case"dir":case"role":case"viewBox":case"width":case"height":Al(e,n,a);break;case"style":Uu(e,a,l);return;case"data":if(t!=="object"){Al(e,"data",a);break}case"src":case"href":if(a===""&&(t!=="a"||n!=="href")){e.removeAttribute(n);break}if(a==null||typeof a=="function"||typeof a=="symbol"||typeof a=="boolean"){e.removeAttribute(n);break}a=ql(a),e.setAttribute(n,a);break;case"action":case"formAction":if(typeof a=="function"){e.setAttribute(n,"javascript:throw new Error('A React form was unexpectedly submitted. If you called form.submit() manually, consider using form.requestSubmit() instead. If you\\'re trying to use event.stopPropagation() in a submit event handler, consider also calling event.preventDefault().')");break}else typeof l=="function"&&(n==="formAction"?(t!=="input"&&Oe(e,t,"name",i.name,i,null),Oe(e,t,"formEncType",i.formEncType,i,null),Oe(e,t,"formMethod",i.formMethod,i,null),Oe(e,t,"formTarget",i.formTarget,i,null)):(Oe(e,t,"encType",i.encType,i,null),Oe(e,t,"method",i.method,i,null),Oe(e,t,"target",i.target,i,null)));if(a==null||typeof a=="symbol"||typeof a=="boolean"){e.removeAttribute(n);break}a=ql(a),e.setAttribute(n,a);break;case"onClick":a!=null&&(e.onclick=tn);return;case"onScroll":a!=null&&de("scroll",e);return;case"onScrollEnd":a!=null&&de("scrollend",e);return;case"dangerouslySetInnerHTML":if(a!=null){if(typeof a!="object"||!("__html"in a))throw Error(d(61));if(n=a.__html,n!=null){if(i.children!=null)throw Error(d(60));(l!=null?l.__html:void 0)!==n&&(e.innerHTML=n)}}break;case"multiple":e.multiple=a&&typeof a!="function"&&typeof a!="symbol";break;case"muted":e.muted=a&&typeof a!="function"&&typeof a!="symbol";break;case"suppressContentEditableWarning":case"suppressHydrationWarning":case"defaultValue":case"defaultChecked":case"innerHTML":case"ref":break;case"autoFocus":break;case"xlinkHref":if(a==null||typeof a=="function"||typeof a=="boolean"||typeof a=="symbol"){e.removeAttribute("xlink:href");break}n=ql(a),e.setAttributeNS("http://www.w3.org/1999/xlink","xlink:href",n);break;case"contentEditable":case"spellCheck":case"draggable":case"value":case"autoReverse":case"externalResourcesRequired":case"focusable":case"preserveAlpha":a!=null&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,a):e.removeAttribute(n);break;case"inert":case"allowFullScreen":case"async":case"autoPlay":case"controls":case"credentialless":case"default":case"defer":case"disabled":case"disablePictureInPicture":case"disableRemotePlayback":case"formNoValidate":case"hidden":case"loop":case"noModule":case"noValidate":case"open":case"playsInline":case"readOnly":case"required":case"reversed":case"scoped":case"seamless":case"itemScope":a&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,""):e.removeAttribute(n);break;case"capture":case"download":a===!0?e.setAttribute(n,""):a!==!1&&a!=null&&typeof a!="function"&&typeof a!="symbol"?e.setAttribute(n,a):e.removeAttribute(n);break;case"cols":case"rows":case"size":case"span":a!=null&&typeof a!="function"&&typeof a!="symbol"&&!isNaN(a)&&1<=a?e.setAttribute(n,a):e.removeAttribute(n);break;case"rowSpan":case"start":a==null||typeof a=="function"||typeof a=="symbol"||isNaN(a)?e.removeAttribute(n):e.setAttribute(n,a);break;case"popover":de("beforetoggle",e),de("toggle",e),Ol(e,"popover",a);break;case"xlinkActuate":pn(e,"http://www.w3.org/1999/xlink","xlink:actuate",a);break;case"xlinkArcrole":pn(e,"http://www.w3.org/1999/xlink","xlink:arcrole",a);break;case"xlinkRole":pn(e,"http://www.w3.org/1999/xlink","xlink:role",a);break;case"xlinkShow":pn(e,"http://www.w3.org/1999/xlink","xlink:show",a);break;case"xlinkTitle":pn(e,"http://www.w3.org/1999/xlink","xlink:title",a);break;case"xlinkType":pn(e,"http://www.w3.org/1999/xlink","xlink:type",a);break;case"xmlBase":pn(e,"http://www.w3.org/XML/1998/namespace","xml:base",a);break;case"xmlLang":pn(e,"http://www.w3.org/XML/1998/namespace","xml:lang",a);break;case"xmlSpace":pn(e,"http://www.w3.org/XML/1998/namespace","xml:space",a);break;case"is":Ol(e,"is",a);break;case"innerText":case"textContent":return;default:if(!(2<n.length)||n[0]!=="o"&&n[0]!=="O"||n[1]!=="n"&&n[1]!=="N")n=qp.get(n)||n,Ol(e,n,a);else return}Ne=!0}function vc(e,t,n,a,i,l){switch(n){case"style":Uu(e,a,l);return;case"dangerouslySetInnerHTML":if(a!=null){if(typeof a!="object"||!("__html"in a))throw Error(d(61));if(n=a.__html,n!=null){if(i.children!=null)throw Error(d(60));(l!=null?l.__html:void 0)!==n&&(e.innerHTML=n)}}break;case"children":if(typeof a=="string")za(e,a);else if(typeof a=="number"||typeof a=="bigint")za(e,""+a);else return;break;case"onScroll":a!=null&&de("scroll",e);return;case"onScrollEnd":a!=null&&de("scrollend",e);return;case"onClick":a!=null&&(e.onclick=tn);return;case"suppressContentEditableWarning":case"suppressHydrationWarning":case"innerHTML":case"ref":return;case"innerText":case"textContent":return;default:if(!ju.hasOwnProperty(n))e:{if(n[0]==="o"&&n[1]==="n"&&(i=n.endsWith("Capture"),l=n.slice(2,i?n.length-7:void 0),t=e[xt]||null,t=t!=null?t[n]:null,typeof t=="function"&&e.removeEventListener(l,t,i),typeof a=="function")){typeof t!="function"&&t!==null&&(n in e?e[n]=null:e.hasAttribute(n)&&e.removeAttribute(n)),e.addEventListener(l,a,i);break e}Ne=!0,n in e?e[n]=a:a===!0?e.setAttribute(n,""):Ol(e,n,a)}return}Ne=!0}function ut(e,t,n){switch(t){case"div":case"span":case"svg":case"path":case"a":case"g":case"p":case"li":break;case"img":de("error",e),de("load",e);var a=!1,i=!1,l;for(l in n)if(n.hasOwnProperty(l)){var s=n[l];if(s!=null)switch(l){case"src":a=!0;break;case"srcSet":i=!0;break;case"children":case"dangerouslySetInnerHTML":throw Error(d(137,t));default:Oe(e,t,l,s,n,null)}}i&&Oe(e,t,"srcSet",n.srcSet,n,null),a&&Oe(e,t,"src",n.src,n,null);return;case"input":de("invalid",e);var u=l=s=i=null,_=null,g=null;for(a in n)if(n.hasOwnProperty(a)){var w=n[a];if(w!=null)switch(a){case"name":i=w;break;case"type":s=w;break;case"checked":_=w;break;case"defaultChecked":g=w;break;case"value":l=w;break;case"defaultValue":u=w;break;case"children":case"dangerouslySetInnerHTML":if(w!=null)throw Error(d(137,t));break;default:Oe(e,t,a,w,n,null)}}Ru(e,l,u,_,g,s,i,!1);return;case"select":de("invalid",e),a=s=l=null;for(i in n)if(n.hasOwnProperty(i)&&(u=n[i],u!=null))switch(i){case"value":l=u;break;case"defaultValue":s=u;break;case"multiple":a=u;default:Oe(e,t,i,u,n,null)}t=l,n=s,e.multiple=!!a,t!=null?Ra(e,!!a,t,!1):n!=null&&Ra(e,!!a,n,!0);return;case"textarea":de("invalid",e),l=i=a=null;for(s in n)if(n.hasOwnProperty(s)&&(u=n[s],u!=null))switch(s){case"value":a=u;break;case"defaultValue":i=u;break;case"children":l=u;break;case"dangerouslySetInnerHTML":if(u!=null)throw Error(d(91));break;default:Oe(e,t,s,u,n,null)}Du(e,a,i,l);return;case"option":for(_ in n)if(n.hasOwnProperty(_)&&(a=n[_],a!=null))switch(_){case"selected":e.selected=a&&typeof a!="function"&&typeof a!="symbol";break;default:Oe(e,t,_,a,n,null)}return;case"dialog":de("beforetoggle",e),de("toggle",e),de("cancel",e),de("close",e);break;case"iframe":case"object":de("load",e);break;case"video":case"audio":for(a=0;a<rl.length;a++)de(rl[a],e);break;case"image":de("error",e),de("load",e);break;case"details":de("toggle",e);break;case"embed":case"source":case"link":de("error",e),de("load",e);case"area":case"base":case"br":case"col":case"hr":case"keygen":case"meta":case"param":case"track":case"wbr":case"menuitem":for(g in n)if(n.hasOwnProperty(g)&&(a=n[g],a!=null))switch(g){case"children":case"dangerouslySetInnerHTML":throw Error(d(137,t));default:Oe(e,t,g,a,n,null)}return;default:if(hs(t)){for(w in n)n.hasOwnProperty(w)&&(a=n[w],a!==void 0&&vc(e,t,w,a,n,void 0));return}}for(u in n)n.hasOwnProperty(u)&&(a=n[u],a!=null&&Oe(e,t,u,a,n,null))}var fv={};function pv(e,t,n,a){switch(t){case"div":case"span":case"svg":case"path":case"a":case"g":case"p":case"li":break;case"input":var i=null,l=null,s=null,u=null,_=null,g=null,w=null;for(b in n){var j=n[b];if(n.hasOwnProperty(b)&&j!=null)switch(b){case"checked":break;case"value":break;case"defaultValue":_=j;default:a.hasOwnProperty(b)||Oe(e,t,b,null,a,j)}}for(var v in a){var b=a[v];if(j=n[v],a.hasOwnProperty(v)&&(b!=null||j!=null))switch(v){case"type":b!==j&&(Ne=!0),l=b;break;case"name":b!==j&&(Ne=!0),i=b;break;case"checked":b!==j&&(Ne=!0),g=b;break;case"defaultChecked":b!==j&&(Ne=!0),w=b;break;case"value":b!==j&&(Ne=!0),s=b;break;case"defaultValue":b!==j&&(Ne=!0),u=b;break;case"children":case"dangerouslySetInnerHTML":if(b!=null)throw Error(d(137,t));break;default:b!==j&&Oe(e,t,v,b,a,j)}}ps(e,s,u,_,g,w,l,i);return;case"select":b=s=u=v=null;for(l in n)if(_=n[l],n.hasOwnProperty(l)&&_!=null)switch(l){case"value":break;case"multiple":b=_;default:a.hasOwnProperty(l)||Oe(e,t,l,null,a,_)}for(i in a)if(l=a[i],_=n[i],a.hasOwnProperty(i)&&(l!=null||_!=null))switch(i){case"value":l!==_&&(Ne=!0),v=l;break;case"defaultValue":l!==_&&(Ne=!0),u=l;break;case"multiple":l!==_&&(Ne=!0),s=l;default:l!==_&&Oe(e,t,i,l,a,_)}t=u,n=s,a=b,v!=null?Ra(e,!!n,v,!1):!!a!=!!n&&(t!=null?Ra(e,!!n,t,!0):Ra(e,!!n,n?[]:"",!1));return;case"textarea":b=v=null;for(u in n)if(i=n[u],n.hasOwnProperty(u)&&i!=null&&!a.hasOwnProperty(u))switch(u){case"value":break;case"children":break;default:Oe(e,t,u,null,a,i)}for(s in a)if(i=a[s],l=n[s],a.hasOwnProperty(s)&&(i!=null||l!=null))switch(s){case"value":i!==l&&(Ne=!0),v=i;break;case"defaultValue":i!==l&&(Ne=!0),b=i;break;case"children":break;case"dangerouslySetInnerHTML":if(i!=null)throw Error(d(91));break;default:i!==l&&Oe(e,t,s,i,a,l)}zu(e,v,b);return;case"option":for(var B in n)if(v=n[B],n.hasOwnProperty(B)&&v!=null&&!a.hasOwnProperty(B))switch(B){case"selected":e.selected=!1;break;default:Oe(e,t,B,null,a,v)}for(_ in a)if(v=a[_],b=n[_],a.hasOwnProperty(_)&&v!==b&&(v!=null||b!=null))switch(_){case"selected":v!==b&&(Ne=!0),e.selected=v&&typeof v!="function"&&typeof v!="symbol";break;default:Oe(e,t,_,v,a,b)}return;case"img":case"link":case"area":case"base":case"br":case"col":case"embed":case"hr":case"keygen":case"meta":case"param":case"source":case"track":case"wbr":case"menuitem":for(var X in n)v=n[X],n.hasOwnProperty(X)&&v!=null&&!a.hasOwnProperty(X)&&Oe(e,t,X,null,a,v);for(g in a)if(v=a[g],b=n[g],a.hasOwnProperty(g)&&v!==b&&(v!=null||b!=null))switch(g){case"children":case"dangerouslySetInnerHTML":if(v!=null)throw Error(d(137,t));break;default:Oe(e,t,g,v,a,b)}return;default:if(hs(t)){for(var se in n)v=n[se],n.hasOwnProperty(se)&&v!==void 0&&!a.hasOwnProperty(se)&&vc(e,t,se,void 0,a,v);for(w in a)v=a[w],b=n[w],!a.hasOwnProperty(w)||v===b||v===void 0&&b===void 0||vc(e,t,w,v,a,b);return}}for(var h in n)v=n[h],n.hasOwnProperty(h)&&v!=null&&!a.hasOwnProperty(h)&&Oe(e,t,h,null,a,v);for(j in a)v=a[j],b=n[j],!a.hasOwnProperty(j)||v===b||v==null&&b==null||Oe(e,t,j,v,a,b)}function Ym(e){switch(e){case"css":case"script":case"font":case"img":case"image":case"input":case"link":return!0;default:return!1}}function vv(){if(typeof performance.getEntriesByType=="function"){for(var e=0,t=0,n=performance.getEntriesByType("resource"),a=0;a<n.length;a++){var i=n[a],l=i.transferSize,s=i.initiatorType,u=i.duration;if(l&&u&&Ym(s)){for(s=0,u=i.responseEnd,a+=1;a<n.length;a++){var _=n[a],g=_.startTime;if(g>u)break;var w=_.transferSize,j=_.initiatorType;w&&Ym(j)&&(_=_.responseEnd,s+=w*(_<u?1:(u-g)/(_-g)))}if(--a,t+=8*(l+s)/(i.duration/1e3),e++,10<e)break}}if(0<e)return t/e/1e6}return navigator.connection&&(e=navigator.connection.downlink,typeof e=="number")?e:5}var hc=null,yc=null;function ol(e){return e.nodeType===9?e:e.ownerDocument}function Im(e){switch(e){case"http://www.w3.org/2000/svg":return 1;case"http://www.w3.org/1998/Math/MathML":return 2;default:return 0}}function Qm(e,t){if(e===0)switch(t){case"svg":return 1;case"math":return 2;default:return 0}return e===1&&t==="foreignObject"?0:e}function Km(e,t,n,a){return n=ol(n).createElement(e),n[lt]=a,n[xt]=t,ut(n,e,t),et(n),n}function gc(e,t){return e==="textarea"||e==="noscript"||typeof t.children=="string"||typeof t.children=="number"||typeof t.children=="bigint"||typeof t.dangerouslySetInnerHTML=="object"&&t.dangerouslySetInnerHTML!==null&&t.dangerouslySetInnerHTML.__html!=null}var xc=null;function hv(){var e=window.event;return e&&e.type==="popstate"?e===xc?!1:(xc=e,!0):(xc=null,!1)}var bc=typeof setTimeout=="function"?setTimeout:void 0,yv=typeof clearTimeout=="function"?clearTimeout:void 0,Xm=typeof Promise=="function"?Promise:void 0,Zm=typeof requestAnimationFrame=="function"?requestAnimationFrame:bc,gv=typeof queueMicrotask=="function"?queueMicrotask:typeof Xm<"u"?function(e){return Xm.resolve(null).then(e).catch(xv)}:bc;function xv(e){setTimeout(function(){throw e})}function Wn(e){return e==="head"}function Pm(e,t){var n=t,a=0;do{var i=n.nextSibling;if(e.removeChild(n),i&&i.nodeType===8)if(n=i.data,n==="/$"||n==="/&"){if(a===0){e.removeChild(i),hi(t);return}a--}else if(n==="$"||n==="$?"||n==="$~"||n==="$!"||n==="&")a++;else if(n==="html")Cc(e.ownerDocument.documentElement);else if(n==="head"){n=e.ownerDocument.head,Cc(n);for(var l=n.firstChild;l;){var s=l.nextSibling,u=l.nodeName;l[Ei]||u==="SCRIPT"||u==="STYLE"||u==="LINK"&&l.rel.toLowerCase()==="stylesheet"||n.removeChild(l),l=s}}else n==="body"&&Cc(e.ownerDocument.body);n=i}while(n);hi(t)}function Fm(e,t){var n=e;e=0;do{var a=n.nextSibling;if(n.nodeType===1?t?(n._stashedDisplay=n.style.display,n.style.display="none"):(n.style.display=n._stashedDisplay||"",n.getAttribute("style")===""&&n.removeAttribute("style")):n.nodeType===3&&(t?(n._stashedText=n.nodeValue,n.nodeValue=""):n.nodeValue=n._stashedText||""),a&&a.nodeType===8)if(n=a.data,n==="/$"){if(e===0)break;e--}else n!=="$"&&n!=="$?"&&n!=="$~"&&n!=="$!"||e++;n=a}while(n)}function Jm(e,t,n){if(t=CSS.escape(t)!==t?"r-"+btoa(t).replace(/=/g,""):t,e.style.viewTransitionName=t,n!=null&&(e.style.viewTransitionClass=n),n=getComputedStyle(e),n.display==="inline"){if(t=e.getClientRects(),t.length===1)var a=1;else for(var i=a=0;i<t.length;i++){var l=t[i];0<l.width&&0<l.height&&a++}a===1&&(e=e.style,e.display=t.length===1?"inline-block":"block",e.marginTop="-"+n.paddingTop,e.marginBottom="-"+n.paddingBottom)}}function Wm(e,t){e=e.style,t=t.style;var n=t!=null?t.hasOwnProperty("viewTransitionName")?t.viewTransitionName:t.hasOwnProperty("view-transition-name")?t["view-transition-name"]:null:null;e.viewTransitionName=n==null||typeof n=="boolean"?"":(""+n).trim(),n=t!=null?t.hasOwnProperty("viewTransitionClass")?t.viewTransitionClass:t.hasOwnProperty("view-transition-class")?t["view-transition-class"]:null:null,e.viewTransitionClass=n==null||typeof n=="boolean"?"":(""+n).trim(),e.display==="inline-block"&&(t==null?e.display=e.margin="":(n=t.display,e.display=n==null||typeof n=="boolean"?"":n,n=t.margin,n!=null?e.margin=n:(n=t.hasOwnProperty("marginTop")?t.marginTop:t["margin-top"],e.marginTop=n==null||typeof n=="boolean"?"":n,t=t.hasOwnProperty("marginBottom")?t.marginBottom:t["margin-bottom"],e.marginBottom=t==null||typeof t=="boolean"?"":t)))}function bv(e,t,n){return n=n.ownerDocument.defaultView,{rect:e,abs:t.position==="absolute"||t.position==="fixed",clip:t.clipPath!=="none"||t.overflow!=="visible"||t.filter!=="none"||t.mask!=="none"||t.mask!=="none"||t.borderRadius!=="0px",view:0<=e.bottom&&0<=e.right&&e.top<=n.innerHeight&&e.left<=n.innerWidth}}function wc(e){var t=e.getBoundingClientRect(),n=getComputedStyle(e);return bv(t,n,e)}function wv(e){return e.documentElement.clientHeight}function kv(e){this.addEventListener("load",e),this.addEventListener("error",e)}function Nv(e,t,n,a,i,l,s,u,_){var g=t.nodeType===9?t:t.ownerDocument;try{var w=g.startViewTransition({update:function(){var v=g.defaultView,b=v.navigation&&v.navigation.transition,B=g.fonts.status;a();var X=[];if(B==="loaded"&&(wv(g),g.fonts.status==="loading"&&X.push(g.fonts.ready)),B=X.length,e!==null)for(var se=e.suspenseyImages,h=0,f=0;f<se.length;f++){var x=se[f];if(!x.complete){var E=x.getBoundingClientRect();if(0<E.bottom&&0<E.right&&E.top<v.innerHeight&&E.left<v.innerWidth){if(h+=xf(x),h>Hr){X.length=B;break}x=new Promise(kv.bind(x)),X.push(x)}}}if(0<X.length)return v=Promise.race([Promise.all(X),new Promise(function(G){return setTimeout(G,500)})]).then(i,i),(b?Promise.allSettled([b.finished,v]):v).then(l,l);if(i(),b)return b.finished.then(l,l);l()},types:n});g.__reactViewTransition=w;var j=[];return w.ready.then(function(){for(var v=g.documentElement.getAnimations({subtree:!0}),b=0;b<v.length;b++){var B=v[b],X=B.effect,se=X.pseudoElement;if(se!=null&&se.startsWith("::view-transition")){j.push(B),B=X.getKeyframes();for(var h=se=void 0,f=!0,x=0;x<B.length;x++){var E=B[x],G=E.width;if(se===void 0)se=G;else if(se!==G){f=!1;break}if(G=E.height,h===void 0)h=G;else if(h!==G){f=!1;break}delete E.width,delete E.height,E.transform==="none"&&delete E.transform}f&&se!==void 0&&h!==void 0&&(X.setKeyframes(B),f=getComputedStyle(X.target,X.pseudoElement),f.width!==se||f.height!==h)&&(f=B[0],f.width=se,f.height=h,f=B[B.length-1],f.width=se,f.height=h,X.setKeyframes(B))}}s()},function(v){g.__reactViewTransition===w&&(g.__reactViewTransition=null);try{if(typeof v=="object"&&v!==null)switch(v.name){case"InvalidStateError":(v.message==="View transition was skipped because document visibility state is hidden."||v.message==="Skipping view transition because document visibility state has become hidden."||v.message==="Skipping view transition because viewport size changed."||v.message==="Transition was aborted because of invalid state")&&(v=null)}v!==null&&_(v)}finally{a(),i(),s()}}),w.finished.finally(function(){for(var v=0;v<j.length;v++)j[v].cancel();g.__reactViewTransition===w&&(g.__reactViewTransition=null),u()}),w}catch{return a(),i(),s(),null}}function Ea(e,t){this._scope=document.documentElement,this._selector="::view-transition-"+e+"("+t+")"}Ea.prototype.animate=function(e,t){return t=typeof t=="number"?{duration:t}:H({},t),t.pseudoElement=this._selector,this._scope.animate(e,t)},Ea.prototype.getAnimations=function(){for(var e=this._scope,t=this._selector,n=e.getAnimations({subtree:!0}),a=[],i=0;i<n.length;i++){var l=n[i].effect;l!==null&&l.target===e&&l.pseudoElement===t&&a.push(n[i])}return a},Ea.prototype.getComputedStyle=function(){return getComputedStyle(this._scope,this._selector)};function $m(e){return{name:e,group:new Ea("group",e),imagePair:new Ea("image-pair",e),old:new Ea("old",e),new:new Ea("new",e)}}function Mt(e){this._fragmentFiber=e,this._observers=this._eventListeners=null}Mt.prototype.addEventListener=function(e,t,n){var a=null,i=null;if(!(n!=null&&typeof n!="boolean"&&(a=n.signal||null,a!==null&&a.aborted))){this._eventListeners===null&&(this._eventListeners=[]);var l=this._eventListeners;if(tf(l,e,t,n)===-1){var s=this,u=t;n!=null&&typeof n!="boolean"&&n.once===!0&&(u=function(_){s.removeEventListener(e,t,n),typeof t=="function"?t.call(this,_):t.handleEvent(_)}),a!==null&&(i=s.removeEventListener.bind(s,e,t,n),a.addEventListener("abort",i,{once:!0}),i=a.removeEventListener.bind(a,"abort",i)),a=di(n),l.push({type:e,listener:t,optionsOrUseCapture:n,attachedListener:u,cleanup:i}),y(this._fragmentFiber.child,!1,Sv,e,u,a)}this._eventListeners=l}};function Sv(e,t,n,a){return J(e).addEventListener(t,n,a),!1}Mt.prototype.removeEventListener=function(e,t,n){var a=this._eventListeners;if(a!==null&&(t=tf(a,e,t,n),t!==-1)){var i=a[t];n=i.attachedListener;var l=i.cleanup;i=di(i.optionsOrUseCapture),y(this._fragmentFiber.child,!1,Ev,e,n,i),a.splice(t,1),l!==null&&l()}};function Ev(e,t,n,a){return J(e).removeEventListener(t,n,a),!1}function di(e){return e!=null&&typeof e!="boolean"&&(e.once===!0||e.signal instanceof AbortSignal)?{capture:e.capture,passive:e.passive}:e}function ef(e){return e==null?"c=0":typeof e=="boolean"?"c="+(e?"1":"0"):"c="+(e.capture?"1":"0")}function tf(e,t,n,a){if(e.length===0)return-1;a=ef(a);for(var i=0;i<e.length;i++){var l=e[i];if(l.type===t&&l.listener===n&&ef(l.optionsOrUseCapture)===a)return i}return-1}Mt.prototype.dispatchEvent=function(e){var t=z(this._fragmentFiber);if(t===null)return!0;t=J(t);var n=this._eventListeners;if(n!==null&&0<n.length||!e.bubbles){var a=t.nodeType===9?t.createComment(""):document.createTextNode("");if(n)for(var i=0;i<n.length;i++){var l=n[i];a.addEventListener(l.type,l.attachedListener,di(l.optionsOrUseCapture))}if(t.appendChild(a),e=a.dispatchEvent(e),n)for(i=0;i<n.length;i++)l=n[i],a.removeEventListener(l.type,l.attachedListener,di(l.optionsOrUseCapture));return t.removeChild(a),e}return t.dispatchEvent(e)},Mt.prototype.focus=function(e){y(this._fragmentFiber.child,!0,nf,e,void 0,void 0)};function nf(e,t){return e.tag===6?!1:(e=J(e),Bv(e,t))}Mt.prototype.focusLast=function(e){var t=[];y(this._fragmentFiber.child,!0,kc,t,void 0,void 0);for(var n=t.length-1;0<=n&&!nf(t[n],e);n--);};function kc(e,t){return t.push(e),!1}Mt.prototype.blur=function(){var e=z(this._fragmentFiber);e!==null&&(e=J(e),e=ol(e).activeElement,e!==null&&y(this._fragmentFiber.child,!1,jv,e,void 0,void 0))};function jv(e,t){return e.tag===6?!1:(e=J(e),e===t||e.contains(t)?(t.blur(),!0):!1)}Mt.prototype.observeUsing=function(e){this._observers===null&&(this._observers=new Set),this._observers.add(e),y(this._fragmentFiber.child,!1,Tv,e,void 0,void 0)};function Tv(e,t){return e.tag===6||(e=J(e),t.observe(e)),!1}Mt.prototype.unobserveUsing=function(e){var t=this._observers;if(t!==null&&t.has(e)){t.delete(e),y(this._fragmentFiber.child,!1,Cv,e,void 0,void 0);for(var n=t=0;n<$t.length;n++){var a=$t[n];a.fragmentInstance===this&&a.observer===e?e.unobserve(a.instance):$t[t++]=a}$t.length=t}};function Cv(e,t){return e.tag===6||(e=J(e),t.unobserve(e)),!1}var $t=[],Nc=!1;function Ov(e,t,n){$t.push({fragmentInstance:e,observer:t,instance:n}),Nc||(Nc=!0,Vv(function(){Nc=!1;var a=$t;$t=[];for(var i=0;i<a.length;i++){var l=a[i];l.observer.unobserve(l.instance)}}))}Mt.prototype.getClientRects=function(){var e=[];return y(this._fragmentFiber.child,!1,Av,e,void 0,void 0),e};function Av(e,t){if(e.tag===6){e=e.stateNode;var n=e.ownerDocument.createRange();n.selectNodeContents(e),t.push.apply(t,n.getClientRects())}else e=J(e),t.push.apply(t,e.getClientRects());return!1}Mt.prototype.getRootNode=function(e){var t=z(this._fragmentFiber);return t===null?this:J(t).getRootNode(e)},Mt.prototype.compareDocumentPosition=function(e){var t=z(this._fragmentFiber);if(t===null)return Node.DOCUMENT_POSITION_DISCONNECTED;var n=[];y(this._fragmentFiber.child,!1,kc,n,void 0,void 0);var a=J(t);if(n.length===0){if(n=a,Y(this._fragmentFiber)){e:{for(t=this._fragmentFiber.return;t!==null;){if(t.tag===4){t=t.stateNode.containerInfo;break e}if(t.tag===3||t.tag===5||t.tag===27)break;t=t.return}t=null}t!=null&&(n=t)}t=this._fragmentFiber;var i=a=n.compareDocumentPosition(e);return n===e?i=Node.DOCUMENT_POSITION_CONTAINS:a&Node.DOCUMENT_POSITION_CONTAINED_BY&&(n=L(t)[1],n===null?i=Node.DOCUMENT_POSITION_PRECEDING:(e=J(n).compareDocumentPosition(e),i=e===0||e&Node.DOCUMENT_POSITION_FOLLOWING?Node.DOCUMENT_POSITION_FOLLOWING:Node.DOCUMENT_POSITION_PRECEDING)),i|=Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC}t=J(n[0]),i=J(n[n.length-1]);var l=Y(this._fragmentFiber)?t.parentElement:a;if(l==null)return Node.DOCUMENT_POSITION_DISCONNECTED;a=l.compareDocumentPosition(t)&Node.DOCUMENT_POSITION_CONTAINED_BY,l=l.compareDocumentPosition(i)&Node.DOCUMENT_POSITION_CONTAINED_BY;var s=t.compareDocumentPosition(e),u=i.compareDocumentPosition(e),_=s&Node.DOCUMENT_POSITION_CONTAINED_BY||u&Node.DOCUMENT_POSITION_CONTAINED_BY;return u=a&&l&&s&Node.DOCUMENT_POSITION_FOLLOWING&&u&Node.DOCUMENT_POSITION_PRECEDING,t=a&&t===e||l&&i===e||_||u?Node.DOCUMENT_POSITION_CONTAINED_BY:!a&&t===e||!l&&i===e?Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC:s,t&Node.DOCUMENT_POSITION_DISCONNECTED||t&Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC||qv(t,this._fragmentFiber,n[0],n[n.length-1],e)?t:Node.DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC};function qv(e,t,n,a,i){var l=la(i);if(e&Node.DOCUMENT_POSITION_CONTAINED_BY){if(n=!!l)e:{for(;l!==null;){if(l.tag===7&&(l===t||l.alternate===t)){n=!0;break e}l=l.return}n=!1}return n}if(e&Node.DOCUMENT_POSITION_CONTAINS){if(l===null)return l=i.ownerDocument,i===l||i===l.documentElement||i===l.body;e:{for(l=t,t=z(t);l!==null;){if(!(l.tag!==5&&l.tag!==3&&l.tag!==27||l!==t&&l.alternate!==t)){l=!0;break e}l=l.return}l=!1}return l}return e&Node.DOCUMENT_POSITION_PRECEDING?((t=!!l)&&!(t=l===n)&&(t=V(n,l,Se),t===null?t=!1:(y(t,!0,Z,l,n),l=K,K=null,t=l!==null)),t):e&Node.DOCUMENT_POSITION_FOLLOWING?((t=!!l)&&!(t=l===a)&&(t=V(a,l,Se),t===null?t=!1:(y(t,!0,_e,l,a),l=K,ae=K=null,t=l!==null)),t):!1}function af(e,t){var n=e.ownerDocument.createRange();n.selectNodeContents(e),e=n.getBoundingClientRect(),window.scrollTo(window.scrollX+e.left,t?window.scrollY+e.top:window.scrollY+e.bottom-window.innerHeight)}Mt.prototype.scrollIntoView=function(e){if(typeof e=="object")throw Error(d(566));var t=[];y(this._fragmentFiber.child,!1,kc,t,void 0,void 0);var n=e!==!1;if(t.length===0){var a=L(this._fragmentFiber);if(a=n?a[1]||a[0]||z(this._fragmentFiber):a[0]||a[1],a===null)return;if(a.tag===6){e=J(a),af(e,n);return}if(a=J(a),a.nodeType!==9){if(a.nodeType===11){n="host"in a?a.host:null,n!==null&&n.scrollIntoView(e);return}a.scrollIntoView(e)}}for(a=n?t.length-1:0;a!==(n?-1:t.length);){var i=t[a];i.tag===6?(i=J(i),af(i,n)):J(i).scrollIntoView(e),a+=n?-1:1}};function Rv(e,t){return e=J(e),lf(e,t),!1}function lf(e,t){e.reactFragments==null&&(e.reactFragments=new Set),e.reactFragments.add(t)}function rf(e,t){var n=t._eventListeners;if(n!==null)for(var a=0;a<n.length;a++){var i=n[a];e.addEventListener(i.type,i.attachedListener,di(i.optionsOrUseCapture))}e.nodeType!==3&&(n=t._observers,n!==null&&n.forEach(function(l){for(var s=0,u=0;u<$t.length;u++){var _=$t[u];(_.fragmentInstance!==t||_.observer!==l||_.instance!==e)&&($t[s++]=_)}$t.length=s,l.observe(e)}),lf(e,t))}function zv(e,t){var n=t._eventListeners;if(n!==null)for(var a=0;a<n.length;a++){var i=n[a];e.removeEventListener(i.type,i.attachedListener,di(i.optionsOrUseCapture))}e.nodeType!==3&&(n=t._observers,n!==null&&n.forEach(function(l){typeof l.rootMargin=="string"?Ov(t,l,e):l.unobserve(e)}),e.reactFragments!=null&&e.reactFragments.delete(t))}function Sc(e){var t=e.firstChild;for(t&&t.nodeType===10&&(t=t.nextSibling);t;){var n=t;switch(t=t.nextSibling,n.nodeName){case"HTML":case"HEAD":case"BODY":Sc(n),Cl(n);continue;case"SCRIPT":case"STYLE":continue;case"LINK":if(n.rel.toLowerCase()==="stylesheet")continue}e.removeChild(n)}}function Dv(e,t,n,a){for(;e.nodeType===1;){var i=n;if(e.nodeName.toLowerCase()!==t.toLowerCase()){if(!a&&(e.nodeName!=="INPUT"||e.type!=="hidden"))break}else if(a){if(!e[Ei])switch(t){case"meta":if(!e.hasAttribute("itemprop"))break;return e;case"link":if(l=e.getAttribute("rel"),l==="stylesheet"&&e.hasAttribute("data-precedence"))break;if(l!==i.rel||e.getAttribute("href")!==(i.href==null||i.href===""?null:i.href)||e.getAttribute("crossorigin")!==(i.crossOrigin==null?null:i.crossOrigin)||e.getAttribute("title")!==(i.title==null?null:i.title))break;return e;case"style":if(e.hasAttribute("data-precedence"))break;return e;case"script":if(l=e.getAttribute("src"),(l!==(i.src==null?null:i.src)||e.getAttribute("type")!==(i.type==null?null:i.type)||e.getAttribute("crossorigin")!==(i.crossOrigin==null?null:i.crossOrigin))&&l&&e.hasAttribute("async")&&!e.hasAttribute("itemprop"))break;return e;default:return e}}else if(t==="input"&&e.type==="hidden"){var l=i.name==null?null:""+i.name;if(i.type==="hidden"&&e.getAttribute("name")===l)return e}else return e;if(e=Qt(e.nextSibling),e===null)break}return null}function Mv(e,t,n){if(t==="")return null;for(;e.nodeType!==3;)if((e.nodeType!==1||e.nodeName!=="INPUT"||e.type!=="hidden")&&!n||(e=Qt(e.nextSibling),e===null))return null;return e}function sf(e,t){for(;e.nodeType!==8;)if((e.nodeType!==1||e.nodeName!=="INPUT"||e.type!=="hidden")&&!t||(e=Qt(e.nextSibling),e===null))return null;return e}function Ec(e){return e.data==="$?"||e.data==="$~"}function jc(e){return e.data==="$!"||e.data==="$?"&&e.ownerDocument.readyState!=="loading"}function Uv(e,t){var n=e.ownerDocument;if(e.data==="$~")e._reactRetry=t;else if(e.data!=="$?"||n.readyState!=="loading")t();else{var a=function(){t(),n.removeEventListener("DOMContentLoaded",a)};n.addEventListener("DOMContentLoaded",a),e._reactRetry=a}}function Qt(e){for(;e!=null;e=e.nextSibling){var t=e.nodeType;if(t===1||t===3)break;if(t===8){if(t=e.data,t==="$"||t==="$!"||t==="$?"||t==="$~"||t==="&"||t==="F!"||t==="F")break;if(t==="/$"||t==="/&")return null}}return e}var Tc=null;function of(e){e=e.nextSibling;for(var t=0;e;){if(e.nodeType===8){var n=e.data;if(n==="/$"||n==="/&"){if(t===0)return Qt(e.nextSibling);t--}else n!=="$"&&n!=="$!"&&n!=="$?"&&n!=="$~"&&n!=="&"||t++}e=e.nextSibling}return null}function cf(e){e=e.previousSibling;for(var t=0;e;){if(e.nodeType===8){var n=e.data;if(n==="$"||n==="$!"||n==="$?"||n==="$~"||n==="&"){if(t===0)return e;t--}else n!=="/$"&&n!=="/&"||t++}e=e.previousSibling}return null}function Bv(e,t){function n(){a=!0}if(e.ownerDocument.activeElement===e)return!0;var a=!1;try{e.ownerDocument.addEventListener("focus",n,!0),(e.focus||HTMLElement.prototype.focus).call(e,t)}finally{e.ownerDocument.removeEventListener("focus",n,!0)}return a}function Vv(e){Zm(function(){Zm(function(t){return e(t)})})}function uf(e,t,n){switch(t=ol(n),e){case"html":if(e=t.documentElement,!e)throw Error(d(452));return e;case"head":if(e=t.head,!e)throw Error(d(453));return e;case"body":if(e=t.body,!e)throw Error(d(454));return e;default:throw Error(d(451))}}function df(e,t,n){for(var a in n){var i=n[a];n.hasOwnProperty(a)&&i!=null&&Oe(e,t,a,null,fv,i)}n.dangerouslySetInnerHTML!=null&&(e.textContent=""),e.onclick===tn&&(e.onclick=null),Cl(e)}function Cc(e){for(var t=e.attributes;t.length;)e.removeAttributeNode(t[0]);Cl(e)}var Kt=new Map,_f=new Set;function cl(e){if(typeof e.getRootNode=="function"){var t=e.getRootNode();if(t.nodeType===9||t.nodeType===11)return t}return e.nodeType===9?e:e.ownerDocument}var Tn=N.d;N.d={f:Lv,r:Hv,D:Gv,C:Yv,L:Iv,m:Qv,X:Xv,S:Kv,M:Zv};function Lv(){var e=Tn.f(),t=qr();return e||t}function Hv(e){var t=Oa(e);t!==null&&t.tag===5&&t.type==="form"?m_(t):Tn.r(e)}var _i=typeof document>"u"?null:document;function mf(e,t,n){var a=_i;if(a&&typeof t=="string"&&t){var i=Bt(t);i='link[rel="'+e+'"][href="'+i+'"]',typeof n=="string"&&(i+='[crossorigin="'+n+'"]'),_f.has(i)||(_f.add(i),e={rel:e,crossOrigin:n,href:t},a.querySelector(i)===null&&(t=a.createElement("link"),ut(t,"link",e),et(t),a.head.appendChild(t)))}}function Gv(e){Tn.D(e),mf("dns-prefetch",e,null)}function Yv(e,t){Tn.C(e,t),mf("preconnect",e,t)}function Iv(e,t,n){Tn.L(e,t,n);var a=_i;if(a&&e&&t){var i='link[rel="preload"][as="'+Bt(t)+'"]';t==="image"&&n&&n.imageSrcSet?(i+='[imagesrcset="'+Bt(n.imageSrcSet)+'"]',typeof n.imageSizes=="string"&&(i+='[imagesizes="'+Bt(n.imageSizes)+'"]')):i+='[href="'+Bt(e)+'"]';var l=i;switch(t){case"style":l=mi(e);break;case"script":l=fi(e)}if(!(Kt.has(l)||(e=H({rel:"preload",href:t==="image"&&n&&n.imageSrcSet?void 0:e,as:t},n),Kt.set(l,e),a.querySelector(i)!==null||t==="style"&&a.querySelector(ul(l))||t==="script"&&a.querySelector(dl(l))))){var s=a.createElement("link");ut(s,"link",e),t==="style"&&(s[Tl]=!0,s.onload=s.onerror=function(){Su(s)}),et(s),a.head.appendChild(s)}}}function Qv(e,t){Tn.m(e,t);var n=_i;if(n&&e){var a=t&&typeof t.as=="string"?t.as:"script",i='link[rel="modulepreload"][as="'+Bt(a)+'"][href="'+Bt(e)+'"]',l=i;switch(a){case"audioworklet":case"paintworklet":case"serviceworker":case"sharedworker":case"worker":case"script":l=fi(e)}if(!Kt.has(l)&&(e=H({rel:"modulepreload",href:e},t),Kt.set(l,e),n.querySelector(i)===null)){switch(a){case"audioworklet":case"paintworklet":case"serviceworker":case"sharedworker":case"worker":case"script":if(n.querySelector(dl(l)))return}a=n.createElement("link"),ut(a,"link",e),et(a),n.head.appendChild(a)}}}function Kv(e,t,n){Tn.S(e,t,n);var a=_i;if(a&&e){var i=Aa(a).hoistableStyles,l=mi(e);t=t||"default";var s=i.get(l);if(!s){var u={loading:0,preload:null};if(s=a.querySelector(ul(l)))u.loading=5;else{e=H({rel:"stylesheet",href:e,"data-precedence":t},n),(n=Kt.get(l))&&Oc(e,n);var _=s=a.createElement("link");et(_),ut(_,"link",e),_._p=new Promise(function(g,w){_.onload=g,_.onerror=w}),_.addEventListener("load",function(){u.loading|=1}),_.addEventListener("error",function(){u.loading|=2}),u.loading|=4,Vr(s,t,a)}s={type:"stylesheet",instance:s,count:1,state:u},i.set(l,s)}}}function Xv(e,t){Tn.X(e,t);var n=_i;if(n&&e){var a=Aa(n).hoistableScripts,i=fi(e),l=a.get(i);l||(l=n.querySelector(dl(i)),l||(e=H({src:e,async:!0},t),(t=Kt.get(i))&&Ac(e,t),l=n.createElement("script"),et(l),ut(l,"link",e),n.head.appendChild(l)),l={type:"script",instance:l,count:1,state:null},a.set(i,l))}}function Zv(e,t){Tn.M(e,t);var n=_i;if(n&&e){var a=Aa(n).hoistableScripts,i=fi(e),l=a.get(i);l||(l=n.querySelector(dl(i)),l||(e=H({src:e,async:!0,type:"module"},t),(t=Kt.get(i))&&Ac(e,t),l=n.createElement("script"),et(l),ut(l,"link",e),n.head.appendChild(l)),l={type:"script",instance:l,count:1,state:null},a.set(i,l))}}function ff(e,t,n,a){var i=(i=$e.current)?cl(i):null;if(!i)throw Error(d(446));switch(e){case"meta":case"title":return null;case"style":return typeof n.precedence=="string"&&typeof n.href=="string"?(n=mi(n.href),t=Aa(i).hoistableStyles,a=t.get(n),a||(a={type:"style",instance:null,count:0,state:null},t.set(n,a)),a):{type:"void",instance:null,count:0,state:null};case"link":if(n.rel==="stylesheet"&&typeof n.href=="string"&&typeof n.precedence=="string"){e=mi(n.href);var l=Aa(i).hoistableStyles,s=l.get(e);if(s||(i=i.ownerDocument||i,s={type:"stylesheet",instance:null,count:0,state:{loading:0,preload:null}},l.set(e,s),(l=i.querySelector(ul(e)))?l._p||(s.instance=l,s.state.loading=5):(l=Kt.get(e),l||(l={rel:"preload",as:"style",href:n.href,crossOrigin:n.crossOrigin,integrity:n.integrity,media:n.media,hrefLang:n.hrefLang,referrerPolicy:n.referrerPolicy},Kt.set(e,l)),Pv(i,e,l,s.state))),t&&a===null)throw Error(d(528,""));return s}if(t&&a!==null)throw Error(d(529,""));return null;case"script":return t=n.async,n=n.src,typeof n=="string"&&t&&typeof t!="function"&&typeof t!="symbol"?(n=fi(n),t=Aa(i).hoistableScripts,a=t.get(n),a||(a={type:"script",instance:null,count:0,state:null},t.set(n,a)),a):{type:"void",instance:null,count:0,state:null};default:throw Error(d(444,e))}}function mi(e){return'href="'+Bt(e)+'"'}function ul(e){return'link[rel="stylesheet"]['+e+"]"}function pf(e){return H({},e,{"data-precedence":e.precedence,precedence:null})}function Pv(e,t,n,a){if(t=e.querySelector('link[rel="preload"][as="style"]['+t+"]")){if(t[Tl]!==!0){a.loading=1;return}}else t=e.createElement("link"),t[Tl]=!0,t.onload=t.onerror=Su.bind(null,t),ut(t,"link",n),et(t),e.head.appendChild(t);a.preload=t,t.addEventListener("load",function(){return a.loading|=1}),t.addEventListener("error",function(){return a.loading|=2})}function fi(e){return'[src="'+Bt(e)+'"]'}function dl(e){return"script[async]"+e}function vf(e,t,n){if(t.count++,t.instance===null)switch(t.type){case"style":var a=e.querySelector('style[data-href~="'+Bt(n.href)+'"]');if(a)return t.instance=a,et(a),a;var i=H({},n,{"data-href":n.href,"data-precedence":n.precedence,href:null,precedence:null});return a=(e.ownerDocument||e).createElement("style"),et(a),ut(a,"style",i),Vr(a,n.precedence,e),t.instance=a;case"stylesheet":i=mi(n.href);var l=e.querySelector(ul(i));if(l)return t.state.loading|=4,t.instance=l,et(l),l;a=pf(n),(i=Kt.get(i))&&Oc(a,i),l=(e.ownerDocument||e).createElement("link"),et(l);var s=l;return s._p=new Promise(function(u,_){s.onload=u,s.onerror=_}),ut(l,"link",a),t.state.loading|=4,Vr(l,n.precedence,e),t.instance=l;case"script":return l=fi(n.src),(i=e.querySelector(dl(l)))?(t.instance=i,et(i),i):(a=n,(i=Kt.get(l))&&(a=H({},n),Ac(a,i)),e=e.ownerDocument||e,i=e.createElement("script"),et(i),ut(i,"link",a),e.head.appendChild(i),t.instance=i);case"void":return null;default:throw Error(d(443,t.type))}else t.type==="stylesheet"&&(t.state.loading&4)===0&&(a=t.instance,t.state.loading|=4,Vr(a,n.precedence,e));return t.instance}function Vr(e,t,n){for(var a=n.querySelectorAll('link[rel="stylesheet"][data-precedence],style[data-precedence]'),i=a.length?a[a.length-1]:null,l=i,s=0;s<a.length;s++){var u=a[s];if(u.dataset.precedence===t)l=u;else if(l!==i)break}l?l.parentNode.insertBefore(e,l.nextSibling):(t=n.nodeType===9?n.head:n,t.insertBefore(e,t.firstChild))}function Oc(e,t){e.crossOrigin==null&&(e.crossOrigin=t.crossOrigin),e.referrerPolicy==null&&(e.referrerPolicy=t.referrerPolicy),e.title==null&&(e.title=t.title)}function Ac(e,t){e.crossOrigin==null&&(e.crossOrigin=t.crossOrigin),e.referrerPolicy==null&&(e.referrerPolicy=t.referrerPolicy),e.integrity==null&&(e.integrity=t.integrity)}var Lr=null;function hf(e,t,n){if(Lr===null){var a=new Map,i=Lr=new Map;i.set(n,a)}else i=Lr,a=i.get(n),a||(a=new Map,i.set(n,a));if(a.has(e))return a;for(a.set(e,null),n=n.getElementsByTagName(e),i=0;i<n.length;i++){var l=n[i];if(!(l[Ei]||l[lt]||e==="link"&&l.getAttribute("rel")==="stylesheet")&&l.namespaceURI!=="http://www.w3.org/2000/svg"){var s=l.getAttribute(t)||"";s=e+s;var u=a.get(s);u?u.push(l):a.set(s,[l])}}return a}function qc(e,t,n){e=e.ownerDocument||e,e.head.insertBefore(n,t==="title"?e.querySelector("head > title"):null)}function Fv(e,t,n){if(n===1||t.itemProp!=null)return!1;switch(e){case"meta":case"title":return!0;case"style":if(typeof t.precedence!="string"||typeof t.href!="string"||t.href==="")break;return!0;case"link":if(typeof t.rel!="string"||typeof t.href!="string"||t.href===""||t.onLoad||t.onError)break;switch(t.rel){case"stylesheet":return e=t.disabled,typeof t.precedence=="string"&&e==null;default:return!0}case"script":if(t.async&&typeof t.async!="function"&&typeof t.async!="symbol"&&!t.onLoad&&!t.onError&&t.src&&typeof t.src=="string")return!0}return!1}function yf(e,t){return e==="img"&&t.src!=null&&t.src!==""&&t.onLoad==null&&t.loading!=="lazy"}function gf(e){return!(e.type==="stylesheet"&&(e.state.loading&3)===0)}function xf(e){return(e.width||100)*(e.height||100)*(typeof devicePixelRatio=="number"?devicePixelRatio:1)*.25}function bf(e,t){typeof t.decode=="function"&&(e.imgCount++,t.complete||(e.imgBytes+=xf(t),e.suspenseyImages.push(t)),e=$v.bind(e),t.decode().then(e,e))}function Jv(e,t,n,a){if(n.type==="stylesheet"&&(typeof a.media!="string"||matchMedia(a.media).matches!==!1)&&(n.state.loading&4)===0){if(n.instance===null){var i=mi(a.href),l=t.querySelector(ul(i));if(l){t=l._p,t!==null&&typeof t=="object"&&typeof t.then=="function"&&(e.count++,e=_l.bind(e),t.then(e,e)),n.state.loading|=4,n.instance=l,et(l);return}l=t.ownerDocument||t,a=pf(a),(i=Kt.get(i))&&Oc(a,i),l=l.createElement("link"),et(l);var s=l;s._p=new Promise(function(u,_){s.onload=u,s.onerror=_}),ut(l,"link",a),n.instance=l}e.stylesheets===null&&(e.stylesheets=new Map),e.stylesheets.set(n,t),(t=n.state.preload)&&(n.state.loading&3)===0&&(e.count++,n=_l.bind(e),t.addEventListener("load",n),t.addEventListener("error",n))}}var Hr=0;function Wv(e,t){return e.stylesheets&&e.count===0&&Yr(e,e.stylesheets),0<e.count||0<e.imgCount?function(n){var a=setTimeout(function(){if(e.stylesheets&&Yr(e,e.stylesheets),e.unsuspend){var l=e.unsuspend;e.unsuspend=null,l()}},6e4+t);0<e.imgBytes&&Hr===0&&(Hr=62500*vv());var i=setTimeout(function(){if(e.waitingForImages=!1,e.count===0&&(e.stylesheets&&Yr(e,e.stylesheets),e.unsuspend)){var l=e.unsuspend;e.unsuspend=null,l()}},(e.imgBytes>Hr?50:800)+t);return e.unsuspend=n,function(){e.unsuspend=null,clearTimeout(a),clearTimeout(i)}}:null}function wf(e){if(e.count===0&&(e.imgCount===0||!e.waitingForImages)){if(e.stylesheets)Yr(e,e.stylesheets);else if(e.unsuspend){var t=e.unsuspend;e.unsuspend=null,t()}}}function _l(){this.count--,wf(this)}function $v(){this.imgCount--,wf(this)}var Gr=null;function Yr(e,t){e.stylesheets=null,e.unsuspend!==null&&(e.count++,Gr=new Map,t.forEach(eh,e),Gr=null,_l.call(e))}function eh(e,t){if(!(t.state.loading&4)){var n=Gr.get(e);if(n)var a=n.get(null);else{n=new Map,Gr.set(e,n);for(var i=e.querySelectorAll("link[data-precedence],style[data-precedence]"),l=0;l<i.length;l++){var s=i[l];(s.nodeName==="LINK"||s.getAttribute("media")!=="not all")&&(n.set(s.dataset.precedence,s),a=s)}a&&n.set(null,a)}i=t.instance,s=i.getAttribute("data-precedence"),l=n.get(s)||a,l===a&&n.set(null,i),n.set(s,i),this.count++,a=_l.bind(this),i.addEventListener("load",a),i.addEventListener("error",a),l?l.parentNode.insertBefore(i,l.nextSibling):(e=e.nodeType===9?e.head:e,e.insertBefore(i,e.firstChild)),t.state.loading|=4}}var pi={$$typeof:Ve,Provider:null,Consumer:null,_currentValue:ge,_currentValue2:ge,_threadCount:0};function th(e,t,n,a,i,l,s,u,_){this.tag=1,this.containerInfo=e,this.pingCache=this.current=this.pendingChildren=null,this.timeoutHandle=-1,this.callbackNode=this.next=this.pendingContext=this.context=this.cancelPendingCommit=null,this.callbackPriority=0,this.expirationTimes=ds(-1),this.entangledLanes=this.shellSuspendCounter=this.errorRecoveryDisabledLanes=this.expiredLanes=this.warmLanes=this.pingedLanes=this.suspendedLanes=this.pendingLanes=0,this.entanglements=ds(0),this.hiddenUpdates=ds(null),this.identifierPrefix=a,this.onUncaughtError=i,this.onCaughtError=l,this.onRecoverableError=s,this.pooledCache=null,this.pooledCacheLanes=0,this.formState=_,this.transitionTypes=null,this.incompleteTransitions=new Map}function kf(e,t,n,a,i,l,s,u,_,g,w,j){return e=new th(e,t,n,s,_,g,w,j,u),t=1,l===!0&&(t|=24),l=bt(3,null,null,t),e.current=l,l.stateNode=e,t=Ks(),t.refCount++,e.pooledCache=t,t.refCount++,l.memoizedState={element:a,isDehydrated:n,cache:t},Fs(l),e}function Nf(e){return e?(e=Ha,e):Ha}function Sf(e,t,n,a,i,l){i=Nf(i),a.context===null?a.context=i:a.pendingContext=i,a=Ln(t),a.payload={element:n},l=l===void 0?null:l,l!==null&&(a.callback=l),n=Hn(e,a,t),n!==null&&(St(n,e,t),Yi(n,e,t))}function Ef(e,t){if(e=e.memoizedState,e!==null&&e.dehydrated!==null){var n=e.retryLane;e.retryLane=n!==0&&n<t?n:t}}function Rc(e,t){Ef(e,t),(e=e.alternate)&&Ef(e,t)}function jf(e){if(e.tag===13||e.tag===31){var t=ca(e,67108864);t!==null&&St(t,e,67108864),Rc(e,67108864)}}function Tf(e){if(e.tag===13||e.tag===31){var t=Dt();t=_s(t);var n=ca(e,t);n!==null&&St(n,e,t),Rc(e,t)}}var vi=!0;function nh(e,t,n,a){var i=R.T;R.T=null;var l=N.p;try{N.p=2,zc(e,t,n,a)}finally{N.p=l,R.T=i}}function ah(e,t,n,a){var i=R.T;R.T=null;var l=N.p;try{N.p=8,zc(e,t,n,a)}finally{N.p=l,R.T=i}}function zc(e,t,n,a){if(vi){var i=Dc(a);if(i===null)pc(e,t,a,Ir,n),Of(e,a);else if(lh(i,e,t,n,a))a.stopPropagation();else if(Of(e,a),t&4&&-1<ih.indexOf(e)){for(;i!==null;){var l=Oa(i);if(l!==null)switch(l.tag){case 3:if(l=l.stateNode,l.current.memoizedState.isDehydrated){var s=ia(l.pendingLanes);if(s!==0){var u=l;for(u.pendingLanes|=2,u.entangledLanes|=2;s;){var _=1<<31-Tt(s);u.entanglements[1]|=_,s&=~_}_n(l),(Ee&6)===0&&(Cr=Et()+500,ll(0))}}break;case 31:case 13:u=ca(l,2),u!==null&&St(u,l,2),qr(),Rc(l,2)}if(l=Dc(a),l===null&&pc(e,t,a,Ir,n),l===i)break;i=l}i!==null&&a.stopPropagation()}else pc(e,t,a,null,n)}}function Dc(e){return e=gs(e),Mc(e)}var Ir=null;function Mc(e){if(Ir=null,e=la(e),e!==null){var t=q(e);if(t===null)e=null;else{var n=t.tag;if(n===13){if(e=k(t),e!==null)return e;e=null}else if(n===31){if(e=M(t),e!==null)return e;e=null}else if(n===3){if(t.stateNode.current.memoizedState.isDehydrated)return t.tag===3?t.stateNode.containerInfo:null;e=null}else t!==e&&(e=null)}}return Ir=e,null}function Cf(e){switch(e){case"beforetoggle":case"cancel":case"click":case"close":case"contextmenu":case"copy":case"cut":case"auxclick":case"dblclick":case"dragend":case"dragstart":case"drop":case"focusin":case"focusout":case"input":case"invalid":case"keydown":case"keypress":case"keyup":case"mousedown":case"mouseup":case"paste":case"pause":case"play":case"pointercancel":case"pointerdown":case"pointerup":case"ratechange":case"reset":case"seeked":case"submit":case"toggle":case"touchcancel":case"touchend":case"touchstart":case"volumechange":case"change":case"selectionchange":case"textInput":case"compositionstart":case"compositionend":case"compositionupdate":case"beforeblur":case"afterblur":case"beforeinput":case"blur":case"fullscreenchange":case"fullscreenerror":case"focus":case"hashchange":case"popstate":case"select":case"selectstart":return 2;case"drag":case"dragenter":case"dragexit":case"dragleave":case"dragover":case"mousemove":case"mouseout":case"mouseover":case"pointermove":case"pointerout":case"pointerover":case"resize":case"scroll":case"touchmove":case"wheel":case"mouseenter":case"mouseleave":case"pointerenter":case"pointerleave":return 8;case"message":switch(vp()){case mu:return 2;case fu:return 8;case kl:case hp:return 32;case pu:return 268435456;default:return 32}default:return 32}}var Uc=!1,$n=null,ea=null,ta=null,ml=new Map,fl=new Map,na=[],ih="mousedown mouseup touchcancel touchend touchstart auxclick dblclick pointercancel pointerdown pointerup dragend dragstart drop compositionend compositionstart keydown keypress keyup input textInput copy cut paste click change contextmenu reset".split(" ");function Of(e,t){switch(e){case"focusin":case"focusout":$n=null;break;case"dragenter":case"dragleave":ea=null;break;case"mouseover":case"mouseout":ta=null;break;case"pointerover":case"pointerout":ml.delete(t.pointerId);break;case"gotpointercapture":case"lostpointercapture":fl.delete(t.pointerId)}}function pl(e,t,n,a,i,l){return e===null||e.nativeEvent!==l?(e={blockedOn:t,domEventName:n,eventSystemFlags:a,nativeEvent:l,targetContainers:[i]},t!==null&&(t=Oa(t),t!==null&&jf(t)),e):(e.eventSystemFlags|=a,t=e.targetContainers,i!==null&&t.indexOf(i)===-1&&t.push(i),e)}function lh(e,t,n,a,i){switch(t){case"focusin":return $n=pl($n,e,t,n,a,i),!0;case"dragenter":return ea=pl(ea,e,t,n,a,i),!0;case"mouseover":return ta=pl(ta,e,t,n,a,i),!0;case"pointerover":var l=i.pointerId;return ml.set(l,pl(ml.get(l)||null,e,t,n,a,i)),!0;case"gotpointercapture":return l=i.pointerId,fl.set(l,pl(fl.get(l)||null,e,t,n,a,i)),!0}return!1}function Af(e){var t=la(e.target);if(t!==null){var n=q(t);if(n!==null){if(t=n.tag,t===13){if(t=k(n),t!==null){e.blockedOn=t,wu(e.priority,function(){Tf(n)});return}}else if(t===31){if(t=M(n),t!==null){e.blockedOn=t,wu(e.priority,function(){Tf(n)});return}}else if(t===3&&n.stateNode.current.memoizedState.isDehydrated){e.blockedOn=n.tag===3?n.stateNode.containerInfo:null;return}}}e.blockedOn=null}function Qr(e){if(e.blockedOn!==null)return!1;for(var t=e.targetContainers;0<t.length;){var n=Dc(e.nativeEvent);if(n===null){n=e.nativeEvent;var a=new n.constructor(n.type,n);ys=a,n.target.dispatchEvent(a),ys=null}else return t=Oa(n),t!==null&&jf(t),e.blockedOn=n,!1;t.shift()}return!0}function qf(e,t,n){Qr(e)&&n.delete(t)}function rh(){Uc=!1,$n!==null&&Qr($n)&&($n=null),ea!==null&&Qr(ea)&&(ea=null),ta!==null&&Qr(ta)&&(ta=null),ml.forEach(qf),fl.forEach(qf)}function Kr(e,t){e.blockedOn===t&&(e.blockedOn=null,Uc||(Uc=!0,o.unstable_scheduleCallback(o.unstable_NormalPriority,rh)))}var Xr=null;function Rf(e){Xr!==e&&(Xr=e,o.unstable_scheduleCallback(o.unstable_NormalPriority,function(){Xr===e&&(Xr=null);for(var t=0;t<e.length;t+=3){var n=e[t],a=e[t+1],i=e[t+2];if(typeof a!="function"){if(Mc(a||n)===null)continue;break}var l=Oa(n);l!==null&&(e.splice(t,3),t-=3,yo(l,{pending:!0,data:i,method:n.method,action:a},a,i))}}))}function hi(e){function t(_){return Kr(_,e)}$n!==null&&Kr($n,e),ea!==null&&Kr(ea,e),ta!==null&&Kr(ta,e),ml.forEach(t),fl.forEach(t);for(var n=0;n<na.length;n++){var a=na[n];a.blockedOn===e&&(a.blockedOn=null)}for(;0<na.length&&(n=na[0],n.blockedOn===null);)Af(n),n.blockedOn===null&&na.shift();if(n=(e.ownerDocument||e).$$reactFormReplay,n!=null)for(a=0;a<n.length;a+=3){var i=n[a],l=n[a+1],s=i[xt]||null;if(typeof l=="function")s||Rf(n);else if(s){var u=null;if(l&&l.hasAttribute("formAction")){if(i=l,s=l[xt]||null)u=s.formAction;else if(Mc(i)!==null)continue}else u=s.action;typeof u=="function"?n[a+1]=u:(n.splice(a,3),a-=3),Rf(n)}}}function zf(){function e(l){l.canIntercept&&l.info==="react-transition"&&l.intercept({handler:function(){return new Promise(function(s){return i=s})},focusReset:"manual",scroll:"manual"})}function t(){i!==null&&(i(),i=null),a||setTimeout(n,20)}function n(){if(!a&&!navigation.transition){var l=navigation.currentEntry;l&&l.url!=null&&navigation.navigate(l.url,{state:l.getState(),info:"react-transition",history:"replace"})}}if(typeof navigation=="object"){var a=!1,i=null;return navigation.addEventListener("navigate",e),navigation.addEventListener("navigatesuccess",t),navigation.addEventListener("navigateerror",t),setTimeout(n,100),function(){a=!0,navigation.removeEventListener("navigate",e),navigation.removeEventListener("navigatesuccess",t),navigation.removeEventListener("navigateerror",t),i!==null&&(i(),i=null)}}}function Bc(e){this._internalRoot=e}Zr.prototype.render=Bc.prototype.render=function(e){var t=this._internalRoot;if(t===null)throw Error(d(409));var n=t.current,a=Dt();Sf(n,a,e,t,null,null)},Zr.prototype.unmount=Bc.prototype.unmount=function(){var e=this._internalRoot;if(e!==null){this._internalRoot=null;var t=e.containerInfo;Sf(e.current,2,null,e,null,null),qr(),t[Ca]=null}};function Zr(e){this._internalRoot=e}Zr.prototype.unstable_scheduleHydration=function(e){if(e){var t=bu();e={blockedOn:null,target:e,priority:t};for(var n=0;n<na.length&&t!==0&&t<na[n].priority;n++);na.splice(n,0,e),n===0&&Af(e)}};var Df=c.version;if(Df!=="19.3.1")throw Error(d(527,Df,"19.3.1"));N.findDOMNode=function(e){var t=e._reactInternals;if(t===void 0)throw typeof e.render=="function"?Error(d(188)):(e=Object.keys(e).join(","),Error(d(268,e)));return e=T(t),e=e!==null?C(e):null,e=e===null?null:e.stateNode,e};var sh={bundleType:0,version:"19.3.1",rendererPackageName:"react-dom",currentDispatcherRef:R,reconcilerVersion:"19.3.1"};if(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__<"u"){var Pr=__REACT_DEVTOOLS_GLOBAL_HOOK__;if(!Pr.isDisabled&&Pr.supportsFiber)try{ki=Pr.inject(sh),jt=Pr}catch{}}return hl.createRoot=function(e,t){if(!S(e))throw Error(d(299));var n=!1,a="",i=k_,l=N_,s=S_;return t!=null&&(t.unstable_strictMode===!0&&(n=!0),t.identifierPrefix!==void 0&&(a=t.identifierPrefix),t.onUncaughtError!==void 0&&(i=t.onUncaughtError),t.onCaughtError!==void 0&&(l=t.onCaughtError),t.onRecoverableError!==void 0&&(s=t.onRecoverableError)),t=kf(e,1,!1,null,null,n,a,null,i,l,s,zf),e[Ca]=t.current,fc(e),new Bc(t)},hl.hydrateRoot=function(e,t,n){if(!S(e))throw Error(d(299));var a=!1,i="",l=k_,s=N_,u=S_,_=null;return n!=null&&(n.unstable_strictMode===!0&&(a=!0),n.identifierPrefix!==void 0&&(i=n.identifierPrefix),n.onUncaughtError!==void 0&&(l=n.onUncaughtError),n.onCaughtError!==void 0&&(s=n.onCaughtError),n.onRecoverableError!==void 0&&(u=n.onRecoverableError),n.formState!==void 0&&(_=n.formState)),t=kf(e,1,!0,t,n??null,a,i,_,l,s,u,zf),t.context=Nf(null),n=t.current,a=Dt(),a=_s(a),i=Ln(a),i.callback=null,Hn(n,i,a),n=a,t.current.lanes=n,Si(t,n),_n(t),e[Ca]=t.current,fc(e),new Zr(t)},hl.version="19.3.1",hl}var Qf;function gh(){if(Qf)return Gc.exports;Qf=1;function o(){if(!(typeof __REACT_DEVTOOLS_GLOBAL_HOOK__>"u"||typeof __REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE!="function"))try{__REACT_DEVTOOLS_GLOBAL_HOOK__.checkDCE(o)}catch(c){console.error(c)}}return o(),Gc.exports=yh(),Gc.exports}var xh=gh();/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const bh=o=>o.replace(/([a-z0-9])([A-Z])/g,"$1-$2").toLowerCase(),$f=(...o)=>o.filter((c,m,d)=>!!c&&c.trim()!==""&&d.indexOf(c)===m).join(" ").trim();/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */var wh={xmlns:"http://www.w3.org/2000/svg",width:24,height:24,viewBox:"0 0 24 24",fill:"none",stroke:"currentColor",strokeWidth:2,strokeLinecap:"round",strokeLinejoin:"round"};/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const kh=P.forwardRef(({color:o="currentColor",size:c=24,strokeWidth:m=2,absoluteStrokeWidth:d,className:S="",children:q,iconNode:k,...M},O)=>P.createElement("svg",{ref:O,...wh,width:c,height:c,stroke:o,strokeWidth:d?Number(m)*24/Number(c):m,className:$f("lucide",S),...M},[...k.map(([T,C])=>P.createElement(T,C)),...Array.isArray(q)?q:[q]]));/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ye=(o,c)=>{const m=P.forwardRef(({className:d,...S},q)=>P.createElement(kh,{ref:q,iconNode:c,className:$f(`lucide-${bh(o)}`,d),...S}));return m.displayName=`${o}`,m};/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Nh=[["path",{d:"M22 12h-2.48a2 2 0 0 0-1.93 1.46l-2.35 8.36a.25.25 0 0 1-.48 0L9.24 2.18a.25.25 0 0 0-.48 0l-2.35 8.36A2 2 0 0 1 4.49 12H2",key:"169zse"}]],xl=ye("Activity",Nh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Sh=[["path",{d:"M20 6 9 17l-5-5",key:"1gmf2c"}]],iu=ye("Check",Sh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Eh=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["line",{x1:"12",x2:"12",y1:"8",y2:"12",key:"1pkeuh"}],["line",{x1:"12",x2:"12.01",y1:"16",y2:"16",key:"4dfq90"}]],jh=ye("CircleAlert",Eh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Th=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["path",{d:"m9 12 2 2 4-4",key:"dzmm74"}]],lu=ye("CircleCheck",Th);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ch=[["rect",{width:"8",height:"4",x:"8",y:"2",rx:"1",ry:"1",key:"tgr4d6"}],["path",{d:"M16 4h2a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2",key:"116196"}],["path",{d:"m9 14 2 2 4-4",key:"df797q"}]],Oh=ye("ClipboardCheck",Ch);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ah=[["circle",{cx:"12",cy:"12",r:"10",key:"1mglay"}],["polyline",{points:"12 6 12 12 16.5 12",key:"1aq6pp"}]],qh=ye("Clock3",Ah);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Rh=[["path",{d:"m18 16 4-4-4-4",key:"1inbqp"}],["path",{d:"m6 8-4 4 4 4",key:"15zrgr"}],["path",{d:"m14.5 4-5 16",key:"e7oirm"}]],ep=ye("CodeXml",Rh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const zh=[["rect",{width:"14",height:"14",x:"8",y:"8",rx:"2",ry:"2",key:"17jyea"}],["path",{d:"M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2",key:"zix9uf"}]],ru=ye("Copy",zh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Dh=[["rect",{width:"16",height:"16",x:"4",y:"4",rx:"2",key:"14l7u7"}],["rect",{width:"6",height:"6",x:"9",y:"9",rx:"1",key:"5aljv4"}],["path",{d:"M15 2v2",key:"13l42r"}],["path",{d:"M15 20v2",key:"15mkzm"}],["path",{d:"M2 15h2",key:"1gxd5l"}],["path",{d:"M2 9h2",key:"1bbxkp"}],["path",{d:"M20 15h2",key:"19e6y8"}],["path",{d:"M20 9h2",key:"19tzq7"}],["path",{d:"M9 2v2",key:"165o2o"}],["path",{d:"M9 20v2",key:"i2bqo8"}]],wi=ye("Cpu",Dh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Mh=[["path",{d:"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4",key:"ih7n3h"}],["polyline",{points:"7 10 12 15 17 10",key:"2ggqvy"}],["line",{x1:"12",x2:"12",y1:"15",y2:"3",key:"1vk2je"}]],Uh=ye("Download",Mh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Bh=[["path",{d:"M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z",key:"1rqfz7"}],["path",{d:"M14 2v4a2 2 0 0 0 2 2h4",key:"tnqrlb"}],["path",{d:"M10 9H8",key:"b1mrlr"}],["path",{d:"M16 13H8",key:"t4e002"}],["path",{d:"M16 17H8",key:"z1uh3a"}]],Vh=ye("FileText",Bh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Lh=[["polygon",{points:"22 3 2 3 10 12.46 10 19 14 21 14 12.46 22 3",key:"1yg77f"}]],Hh=ye("Filter",Lh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Gh=[["path",{d:"M12 10a2 2 0 0 0-2 2c0 1.02-.1 2.51-.26 4",key:"1nerag"}],["path",{d:"M14 13.12c0 2.38 0 6.38-1 8.88",key:"o46ks0"}],["path",{d:"M17.29 21.02c.12-.6.43-2.3.5-3.02",key:"ptglia"}],["path",{d:"M2 12a10 10 0 0 1 18-6",key:"ydlgp0"}],["path",{d:"M2 16h.01",key:"1gqxmh"}],["path",{d:"M21.8 16c.2-2 .131-5.354 0-6",key:"drycrb"}],["path",{d:"M5 19.5C5.5 18 6 15 6 12a6 6 0 0 1 .34-2",key:"1tidbn"}],["path",{d:"M8.65 22c.21-.66.45-1.32.57-2",key:"13wd9y"}],["path",{d:"M9 6.8a6 6 0 0 1 9 5.2v2",key:"1fr1j5"}]],Jc=ye("Fingerprint",Gh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Yh=[["path",{d:"M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z",key:"96xj49"}]],su=ye("Flame",Yh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ih=[["path",{d:"m12 14 4-4",key:"9kzdfg"}],["path",{d:"M3.34 19a10 10 0 1 1 17.32 0",key:"19p75a"}]],$r=ye("Gauge",Ih);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Qh=[["circle",{cx:"12",cy:"12",r:"3",key:"1v7zrd"}],["line",{x1:"3",x2:"9",y1:"12",y2:"12",key:"1dyftd"}],["line",{x1:"15",x2:"21",y1:"12",y2:"12",key:"oup4p8"}]],Kh=ye("GitCommitHorizontal",Qh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Xh=[["line",{x1:"22",x2:"2",y1:"12",y2:"12",key:"1y58io"}],["path",{d:"M5.45 5.11 2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z",key:"oot6mr"}],["line",{x1:"6",x2:"6.01",y1:"16",y2:"16",key:"sgf278"}],["line",{x1:"10",x2:"10.01",y1:"16",y2:"16",key:"1l4acy"}]],es=ye("HardDrive",Xh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Zh=[["path",{d:"M12.83 2.18a2 2 0 0 0-1.66 0L2.6 6.08a1 1 0 0 0 0 1.83l8.58 3.91a2 2 0 0 0 1.66 0l8.58-3.9a1 1 0 0 0 0-1.83z",key:"zw3jo"}],["path",{d:"M2 12a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 12",key:"1wduqc"}],["path",{d:"M2 17a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 17",key:"kqbvx6"}]],tp=ye("Layers",Zh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Ph=[["circle",{cx:"12",cy:"16",r:"1",key:"1au0dj"}],["rect",{x:"3",y:"10",width:"18",height:"12",rx:"2",key:"6s8ecr"}],["path",{d:"M7 10V7a5 5 0 0 1 10 0v3",key:"1pqi11"}]],ou=ye("LockKeyhole",Ph);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Fh=[["path",{d:"M18 8V6a2 2 0 0 0-2-2H4a2 2 0 0 0-2 2v7a2 2 0 0 0 2 2h8",key:"10dyio"}],["path",{d:"M10 19v-3.96 3.15",key:"1irgej"}],["path",{d:"M7 19h5",key:"qswx4l"}],["rect",{width:"6",height:"10",x:"16",y:"12",rx:"2",key:"1egngj"}]],Jh=ye("MonitorSmartphone",Fh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const Wh=[["path",{d:"M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z",key:"a7tn18"}]],$h=ye("Moon",Wh);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ey=[["polygon",{points:"6 3 20 12 6 21 6 3",key:"1oa8hb"}]],ty=ye("Play",ey);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ny=[["path",{d:"M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8",key:"v9h5vc"}],["path",{d:"M21 3v5h-5",key:"1q7to0"}],["path",{d:"M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16",key:"3uifl3"}],["path",{d:"M8 16H3v5",key:"1cv678"}]],np=ye("RefreshCw",ny);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ay=[["path",{d:"M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8",key:"1357e3"}],["path",{d:"M3 3v5h5",key:"1xhq8a"}]],ap=ye("RotateCcw",ay);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const iy=[["circle",{cx:"11",cy:"11",r:"8",key:"4ej97u"}],["path",{d:"m21 21-4.3-4.3",key:"1qie3q"}]],ip=ye("Search",iy);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ly=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}],["path",{d:"M12 8v4",key:"1got3b"}],["path",{d:"M12 16h.01",key:"1drbdi"}]],xi=ye("ShieldAlert",ly);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const ry=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}],["path",{d:"m9 12 2 2 4-4",key:"dzmm74"}]],ja=ye("ShieldCheck",ry);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const sy=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z",key:"oel41y"}]],oy=ye("Shield",sy);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const cy=[["line",{x1:"4",x2:"4",y1:"21",y2:"14",key:"1p332r"}],["line",{x1:"4",x2:"4",y1:"10",y2:"3",key:"gb41h5"}],["line",{x1:"12",x2:"12",y1:"21",y2:"12",key:"hf2csr"}],["line",{x1:"12",x2:"12",y1:"8",y2:"3",key:"1kfi7u"}],["line",{x1:"20",x2:"20",y1:"21",y2:"16",key:"1lhrwl"}],["line",{x1:"20",x2:"20",y1:"12",y2:"3",key:"16vvfq"}],["line",{x1:"2",x2:"6",y1:"14",y2:"14",key:"1uebub"}],["line",{x1:"10",x2:"14",y1:"8",y2:"8",key:"1yglbp"}],["line",{x1:"18",x2:"22",y1:"16",y2:"16",key:"1jxqpz"}]],uy=ye("SlidersVertical",cy);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const dy=[["circle",{cx:"12",cy:"12",r:"4",key:"4exip2"}],["path",{d:"M12 2v2",key:"tus03m"}],["path",{d:"M12 20v2",key:"1lh1kg"}],["path",{d:"m4.93 4.93 1.41 1.41",key:"149t6j"}],["path",{d:"m17.66 17.66 1.41 1.41",key:"ptbguv"}],["path",{d:"M2 12h2",key:"1t8f8n"}],["path",{d:"M20 12h2",key:"1q8mjw"}],["path",{d:"m6.34 17.66-1.41 1.41",key:"1m8zz5"}],["path",{d:"m19.07 4.93-1.41 1.41",key:"1shlcs"}]],_y=ye("Sun",dy);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const my=[["polyline",{points:"4 17 10 11 4 5",key:"akl6gq"}],["line",{x1:"12",x2:"20",y1:"19",y2:"19",key:"q2wloq"}]],cu=ye("Terminal",my);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const fy=[["path",{d:"M14 4v10.54a4 4 0 1 1-4 0V4a2 2 0 0 1 4 0Z",key:"17jzev"}]],Wc=ye("Thermometer",fy);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const py=[["path",{d:"m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3",key:"wmoenq"}],["path",{d:"M12 9v4",key:"juzpu7"}],["path",{d:"M12 17h.01",key:"p32p05"}]],vy=ye("TriangleAlert",py);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const hy=[["path",{d:"M18 6 6 18",key:"1bl5f8"}],["path",{d:"m6 6 12 12",key:"d8bk6v"}]],lp=ye("X",hy);/**
 * @license lucide-react v0.475.0 - ISC
 *
 * This source code is licensed under the ISC license.
 * See the LICENSE file in the root directory of this source tree.
 */const yy=[["path",{d:"M4 14a1 1 0 0 1-.78-1.63l9.9-10.2a.5.5 0 0 1 .86.46l-1.92 6.02A1 1 0 0 0 13 10h7a1 1 0 0 1 .78 1.63l-9.9 10.2a.5.5 0 0 1-.86-.46l1.92-6.02A1 1 0 0 0 11 14z",key:"1xq2db"}]],rp=ye("Zap",yy),sp="coreflow.appearance.v1";function gy(){var o;try{const c=JSON.parse(((o=window.localStorage)==null?void 0:o.getItem(sp))||"{}");return{theme:["system","light","dark"].includes(c.theme)?c.theme:"system",reduceMotion:c.reduceMotion===!0}}catch{return{theme:"system",reduceMotion:!1}}}function op(o){var m;const c=o.theme==="dark"||o.theme==="system"&&((m=window.matchMedia)==null?void 0:m.call(window,"(prefers-color-scheme: dark)").matches);document.documentElement.dataset.theme=c?"dark":"light",document.documentElement.dataset.motion=o.reduceMotion?"reduce":"system"}function xy(o){op(o);let c=!0;try{window.localStorage.setItem(sp,JSON.stringify(o))}catch{c=!1}return window.dispatchEvent(new CustomEvent("coreflow-appearance",{detail:o})),c}const Kf=1e4,by=12e4;let Xf=0;function as(){var o;return typeof window<"u"&&typeof((o=window.ksu)==null?void 0:o.exec)=="function"}function Kc(o){if(typeof o=="string")return{errno:0,stdout:o,stderr:""};if(typeof o=="object"&&o!==null){const c=o,m=typeof c.errno=="number"&&Number.isFinite(c.errno)?c.errno:-1,d=typeof c.stdout=="string"?c.stdout:"",S=typeof c.stderr=="string"?c.stderr:"";return{errno:m,stdout:d,stderr:S}}return{errno:-1,stdout:"",stderr:"Malformed native bridge response"}}async function bi(o,c=Kf){if(!as()||!window.ksu)return{errno:-1,stdout:"",stderr:"Native KernelSU bridge unavailable"};if(typeof o!="string"||!o||o.includes("\0"))return{errno:-1,stdout:"",stderr:"Invalid shell command"};const m=window.ksu,d=Number.isFinite(c)?Math.min(by,Math.max(1e3,c)):Kf;return new Promise(S=>{let q=!1;Xf+=1;const k=`__coreflow_ksu_cb_${Date.now()}_${Xf}`,M=T=>{q||(q=!0,window.clearTimeout(O),delete window[k],S(T))},O=window.setTimeout(()=>{M({errno:-1,stdout:"",stderr:`Native command timed out after ${d}ms`})},d);window[k]=(T,C,y)=>{if(typeof T=="object"&&T!==null){M(Kc(T));return}M({errno:typeof T=="number"&&Number.isFinite(T)?T:0,stdout:typeof C=="string"?C:String(C??""),stderr:typeof y=="string"?y:String(y??"")})};try{const T=m.exec(o,"{}",k);T!==void 0&&Promise.resolve(T).then(C=>M(Kc(C))).catch(C=>M({errno:-1,stdout:"",stderr:C instanceof Error?C.message:String(C)}))}catch{try{const T=m.exec(o);Promise.resolve(T).then(C=>M(Kc(C))).catch(C=>M({errno:-1,stdout:"",stderr:C instanceof Error?C.message:String(C)}))}catch(T){M({errno:-1,stdout:"",stderr:T instanceof Error?T.message:String(T)})}}})}const $c={mode:"disabled",enableSkiavkThreaded:!1,enableAdaptiveVrs:!1,enablePipelineCachePrewarm:!1};function gl(o,c){const m=o.mode==="full_capability_override"?"safe_hardware":o.mode,d=m!=="disabled",S=(c==null?void 0:c.deviceExtensions)??0,q=(c==null?void 0:c.hwRayTracing)??(c==null?void 0:c.rayTracing)??!1,k=(c==null?void 0:c.hwMeshShaders)??(c==null?void 0:c.meshShaders)??!1,M=(c==null?void 0:c.hwVariableRateShading)??(c==null?void 0:c.variableRateShading)??!1,O=(c==null?void 0:c.hwCooperativeMatrix)??(c==null?void 0:c.cooperativeMatrix)??!1,T=(c==null?void 0:c.hwRtPipeline)??(c==null?void 0:c.rtPipeline)??!1,C=(c==null?void 0:c.hwRayQuery)??(c==null?void 0:c.rayQuery)??!1,y=(c==null?void 0:c.hwAccelerationStructure)??(c==null?void 0:c.accelerationStructure)??!1,z=(c==null?void 0:c.hwShaderFloat64)??(c==null?void 0:c.shaderFloat64)??!1,Y=(c==null?void 0:c.hwIndexTypeUint8)??(c==null?void 0:c.indexTypeUint8)??!1,L=(c==null?void 0:c.hwLineRasterization)??(c==null?void 0:c.lineRasterization)??!1,I=(c==null?void 0:c.hwPushDescriptors)??(c==null?void 0:c.pushDescriptors)??!1,J=(c==null?void 0:c.hwGlobalPriorityQuery)??(c==null?void 0:c.globalPriorityQuery)??!1,K=(c==null?void 0:c.hwShaderFloatControls2)??(c==null?void 0:c.shaderFloatControls2)??!1,ae=(c==null?void 0:c.hwMaintenance5)??(c==null?void 0:c.maintenance5)??!1,Z=(c==null?void 0:c.hwMaintenance6)??(c==null?void 0:c.maintenance6)??!1,_e=(c==null?void 0:c.hwMaintenance7)??(c==null?void 0:c.maintenance7)??!1,Se=(c==null?void 0:c.hwDescriptorBuffer)??(c==null?void 0:c.descriptorBuffer)??!1,V=(c==null?void 0:c.hwGraphicsPipelineLibrary)??(c==null?void 0:c.graphicsPipelineLibrary)??!1,H=(c==null?void 0:c.apiVersion)??"Probing ICD...";return{available:(c==null?void 0:c.available)??!1,layerInstalled:(c==null?void 0:c.layerInstalled)??!0,layerActive:d,mode:m,vendor:(c==null?void 0:c.vendor)??"Auto-Detecting GPU Vendor",deviceName:(c==null?void 0:c.deviceName)??"Probing Physical GPU ICD...",apiVersion:H,effectiveApiVersion:H,driverVersion:(c==null?void 0:c.driverVersion)??"0.0.0",deviceExtensions:S,effectiveDeviceExtensions:S,instanceExtensions:(c==null?void 0:c.instanceExtensions)??0,deviceUuid:(c==null?void 0:c.deviceUuid)??"00000000-0000-0000-0000-000000000000",driverUuid:(c==null?void 0:c.driverUuid)??"00000000-0000-0000-0000-000000000000",pipelineCacheUuid:(c==null?void 0:c.pipelineCacheUuid)??"00000000-0000-0000-0000-000000000000",rayTracing:q,meshShaders:k,variableRateShading:M,cooperativeMatrix:O,rtPipeline:T,rayQuery:C,accelerationStructure:y,portabilitySubset:(c==null?void 0:c.portabilitySubset)??!1,robustBufferAccess:(c==null?void 0:c.robustBufferAccess)??!0,samplerAnisotropy:(c==null?void 0:c.samplerAnisotropy)??!0,shaderInt16:(c==null?void 0:c.shaderInt16)??!0,shaderFloat64:z,protectedMemory:(c==null?void 0:c.protectedMemory)??!0,timelineSemaphore:(c==null?void 0:c.timelineSemaphore)??!0,descriptorIndexing:(c==null?void 0:c.descriptorIndexing)??!0,bufferDeviceAddress:(c==null?void 0:c.bufferDeviceAddress)??!0,dynamicRendering:(c==null?void 0:c.dynamicRendering)??!0,synchronization2:(c==null?void 0:c.synchronization2)??!0,indexTypeUint8:Y,lineRasterization:L,pushDescriptors:I,globalPriorityQuery:J,shaderFloatControls2:K,maxPushDescriptors:(c==null?void 0:c.maxPushDescriptors)??(I?32:0),maintenance5:ae,maintenance6:Z,maintenance7:_e,descriptorBuffer:Se,graphicsPipelineLibrary:V,hwRayTracing:q,hwMeshShaders:k,hwVariableRateShading:M,hwCooperativeMatrix:O,hwRtPipeline:T,hwRayQuery:C,hwAccelerationStructure:y,hwShaderFloat64:z,hwIndexTypeUint8:Y,hwLineRasterization:L,hwPushDescriptors:I,hwGlobalPriorityQuery:J,hwShaderFloatControls2:K,hwMaintenance5:ae,hwMaintenance6:Z,hwMaintenance7:_e,hwDescriptorBuffer:Se,hwGraphicsPipelineLibrary:V,skiavkActive:d&&o.enableSkiavkThreaded,adaptiveVrsActive:d&&o.enableAdaptiveVrs&&M,pipelineCachePrewarmActive:d&&o.enablePipelineCachePrewarm,gpuKernelAvailable:(c==null?void 0:c.gpuKernelAvailable)??!1,gpuKernelLabel:(c==null?void 0:c.gpuKernelLabel)??"Universal Multi-Vendor GPU Devfreq",gpuGovernor:(c==null?void 0:c.gpuGovernor)??"auto",gpuCurrentFreqMhz:(c==null?void 0:c.gpuCurrentFreqMhz)??0,gpuMaxFreqMhz:(c==null?void 0:c.gpuMaxFreqMhz)??0,adrenoboostLevel:(c==null?void 0:c.adrenoboostLevel)??0}}const Zf={version:"general",log_level:"general",zero_polling_mode:"general",enable_cgroups_v2:"general",enable_uclamp:"general",enable_tuner:"general",tier_override:"device_adaptation",unlock_vendor_clamping:"device_adaptation",force_120hz_rendering:"device_adaptation",enable_cpu_frequency_control:"device_adaptation",enable_gpu_frequency_control:"device_adaptation",enable_direct_adaptive:"direct_adaptive",direct_adaptive_policy:"direct_adaptive",top_app_interactive_min:"uclamp_tuning",top_app_interactive_max:"uclamp_tuning",ui_adaptive_bias:"uclamp_tuning",ui_balanced_uclamp_min:"uclamp_tuning",foreground_boost_min:"uclamp_tuning",background_clamp_max:"uclamp_tuning",system_background_max:"uclamp_tuning",temp_monitor_interval_ms:"thermal_guardian",thermal_trip_celsius:"thermal_guardian",thermal_recover_celsius:"thermal_guardian",emergency_rollback_on_overheat:"thermal_guardian",cpu_pressure_boost_pct:"stability_policy",memory_pressure_guard_pct:"stability_policy",io_pressure_guard_pct:"stability_policy",thermal_headroom_celsius:"stability_policy",thermal_emergency_margin_celsius:"stability_policy",enable_pid_equilibrium:"pid_equilibrium",pid_target_celsius:"pid_equilibrium",pid_kp:"pid_equilibrium",pid_ki:"pid_equilibrium",pid_kd:"pid_equilibrium",enable_battery_step_charging:"hardware_longevity",enable_ufs_burst_pacing:"hardware_longevity",battery_step_temp_threshold:"hardware_longevity",stepped_charge_current_ma:"hardware_longevity",top_app_io_weight:"hardware_longevity",background_io_weight:"hardware_longevity",vulkan_layer_mode:"vulkan_engine",enable_skiavk_threaded:"vulkan_engine",enable_adaptive_vrs:"vulkan_engine",enable_pipeline_cache_prewarm:"vulkan_engine",target_fps:"render_booster",pre_boost_on_touch:"render_booster",boost_duration_ms:"render_booster"};function gi(o){return Object.prototype.hasOwnProperty.call(Zf,o)?Zf[o]:void 0}function cp(o){const c=o.trim();return c==="stability"?"stability_policy":c}function wy(o){const c=Object.create(null);let m="";for(const d of o.split(/\r?\n/)){const S=d.trim();if(!S||S.startsWith("#")||S.startsWith(";"))continue;if(S.startsWith("[")&&S.endsWith("]")){m=cp(S.slice(1,-1)),c[m]||(c[m]=Object.create(null));continue}const q=S.indexOf("=");if(q<0)continue;const k=S.slice(0,q).trim(),M=S.slice(q+1).trim();if(!k)continue;const O=gi(k);if(O&&m!==O)continue;const T=m||O||"general";c[T]||(c[T]=Object.create(null)),c[T][k]=M}return c}function eu(o,c){var S;const m=wy(o),d=gi(c);if(d){const q=(S=m[d])==null?void 0:S[c];return q&&q.length>0?q:null}for(const q of Object.values(m))if(q[c])return q[c];return null}function ky(o,c){const m=eu(o,c);if(m===null)return null;const d=Number(m);return Number.isFinite(d)?d:null}function Ny(o,c){const m={};for(const[O,T]of Object.entries(c)){const C=O.trim();if(!C||gi(C)===void 0)continue;const y=String(T).trim();/[\r\n\0]/.test(y)||(m[C]=y)}if(Object.keys(m).length===0)return o;const d=o?o.replace(/\r\n/g,`
`).split(`
`):[];d.length>0&&d[d.length-1]===""&&d.pop();const S=[];let q="";const k=O=>{if(O)for(const[T,C]of Object.entries(m))gi(T)===O&&(S.push(`${T} = ${C}`),delete m[T])};for(const O of d){const T=O.trim();if(T.startsWith("[")&&T.endsWith("]")){k(q),q=cp(T.slice(1,-1)),S.push(O);continue}if(!T||T.startsWith("#")||T.startsWith(";")){S.push(O);continue}const C=O.indexOf("=");if(C<0){S.push(O);continue}const y=O.slice(0,C).trim();Object.prototype.hasOwnProperty.call(m,y)&&gi(y)===q?(S.push(`${y} = ${m[y]}`),delete m[y]):S.push(O)}k(q);const M=["general","device_adaptation","direct_adaptive","uclamp_tuning","thermal_guardian","stability_policy","pid_equilibrium","hardware_longevity","vulkan_engine","render_booster"];for(const O of M){const T=Object.keys(m).filter(C=>gi(C)===O);if(T.length!==0){S.length>0&&S[S.length-1].trim()!==""&&S.push(""),S.push(`[${O}]`);for(const C of T)S.push(`${C} = ${m[C]}`),delete m[C]}}return S.join(`
`)+`
`}const Xc=o=>typeof o=="object"&&o!==null&&!Array.isArray(o);function Sy(o){if(!Xc(o))return!1;const c=["cpuUtilizationPct","thermalPressure","ramBytes","zramBytes","refreshHz","discoveredNodes","baselineNodes","verifiedMutations","skippedNodes","ownershipLosses"],m=["mode","screenState","gpuModel","storageType"];return!(typeof o.active!="boolean"||typeof o.discoveryComplete!="boolean"||typeof o.thermalGuard!="boolean"||typeof o.journalFault!="boolean"||!c.every(d=>typeof o[d]=="number"&&Number.isFinite(o[d]))||!m.every(d=>typeof o[d]=="string")||!Array.isArray(o.cpuClusters)||!o.cpuClusters.every(d=>Xc(d)&&typeof d.id=="string"&&Array.isArray(d.opps)&&d.opps.every(S=>typeof S=="string"))||!Array.isArray(o.appliedValues)||!o.appliedValues.every(d=>Xc(d)&&["semantic","domain","path","current","oemBaseline","target","health"].every(S=>typeof d[S]=="string")&&typeof d.modified=="boolean"&&typeof d.owned=="boolean"))}const ts={thermalTripCelsius:44,balancedUclampMin:320};function uu(o){switch(o){case"sustained_load":return{label:"Sustained Load",description:"Coordinated CPU responsiveness transaction active with verified thermal headroom.",stage:"OBSERVING",risk:"Medium",outcome:"IMPROVED"};case"pressure_guard":return{label:"Pressure Guard",description:"Memory or I/O contention elevated; dynamic block scheduler & VM relief engaged.",stage:"OBSERVING",risk:"Low",outcome:"IMPROVED"};case"thermal_guard":return{label:"Thermal Guard",description:"OEM/Android thermal safety gate engaged; active mitigation pacing SoC demand.",stage:"VERIFIED",risk:"Low",outcome:"ABORTED"};case"balanced":default:return{label:"Balanced Equilibrium",description:"Coordinated daily baseline retained with readback-verified top-app UClamp floor.",stage:"RETAINED",risk:"Low",outcome:"IMPROVED"}}}function Ey(){const o=uu("balanced");return{mode:"balanced",modeLabel:o.label,modeDescription:o.description,engineActive:!0,uclampMin:320,uclampMax:1024,uclampSupported:!0,socTempCelsius:38.6,socCompositeCelsius:38.6,cpuAverageCelsius:38.2,cpuPeakCelsius:41.5,gpuTempCelsius:36.8,batteryTempCelsius:33.4,skinTempCelsius:35.1,thermalSensors:[{id:"tz0",type:"cpu-0-0",category:1,celsius:37.8,valid:!0,policyAvailable:!0},{id:"tz1",type:"cpu-0-1",category:1,celsius:38,valid:!0,policyAvailable:!0},{id:"tz2",type:"cpu-1-0",category:1,celsius:39.4,valid:!0,policyAvailable:!0},{id:"tz3",type:"cpu-1-2",category:1,celsius:41.5,valid:!0,policyAvailable:!0},{id:"tz4",type:"gpuss-0",category:1,celsius:36.8,valid:!0,policyAvailable:!0},{id:"tz5",type:"quiet_therm",category:3,celsius:35.1,valid:!0,policyAvailable:!0},{id:"tz6",type:"battery",category:2,celsius:33.4,valid:!0,policyAvailable:!0},{id:"tz7",type:"ap_therm",category:4,celsius:37,valid:!0,policyAvailable:!0}],cpuPressurePct:1.2,memoryPressurePct:.3,ioPressurePct:.4,cpuPressureValid:!0,memoryPressureValid:!0,ioPressureValid:!0,baselineNodesSnapshotted:6,baselineStatus:"protected",transactionStage:"RETAINED",riskClass:"Low",lastOutcome:"IMPROVED",ownershipIntact:!0,aiBackend:"embedded-rls-mlp",aiOnnxActive:!1,aiPredictedPressure10s:46.2,aiPredictedVelocity:.08,aiRecommendedScale:.94,aiConfidence:.86,aiSamples:12,tunerStage:"RETAINED",tunerIntent:"HoldEquilibrium",tunerCandidateId:"baseline",tunerCandidateLabel:"OEM Verified Baseline",tunerRationale:"System in equilibrium (I/O & memory PSI low); retaining OEM block scheduler without unnecessary mutation.",tunerVerdict:"IMPROVED",tunerUtilityDelta:0,tunerTrials:3,tunerImproved:2,tunerRolledBack:1,blockQueues:[{device:"sda",activeScheduler:"mq-deadline",baselineScheduler:"none",availableSchedulers:["none","mq-deadline","kyber","bfq"],readAheadKb:128,nrRequests:128,rqAffinity:2,readExpireMs:250,writesStarved:3,fifoBatch:8}],tunerCandidates:[{id:"io.mq_deadline_low_latency",label:"mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",domain:"io",trials:2,successes:2,rollbacks:0,meanDelta:7.4,ucbScore:8.12,cooldownTicks:0},{id:"io.kyber_token_bucket",label:"kyber Token-Bucket Low-Latency I/O Scheduler",domain:"io",trials:1,successes:0,rollbacks:1,meanDelta:-1.8,ucbScore:.42,cooldownTicks:0},{id:"cpufreq.schedutil_interactive_response",label:"EAS Governor Rate-Limit Pacing (500us up / 10000us down)",domain:"cpufreq",trials:1,successes:1,rollbacks:0,meanDelta:5.2,ucbScore:6.18,cooldownTicks:0},{id:"vm.zram_latency_relief",label:"VM Page-Cluster Zero-Latency & VFS Dentry Preservation",domain:"vm",trials:1,successes:1,rollbacks:0,meanDelta:4.1,ucbScore:5.15,cooldownTicks:0}],vulkan:gl($c,{available:!0,layerInstalled:!0,vendor:"Multi-Vendor Preview (Adreno / Mali / Xclipse)",deviceName:"Universal Mobile GPU Preview Profile",apiVersion:"1.3.280",driverVersion:"512.762.0",deviceExtensions:148,instanceExtensions:16,deviceUuid:"cf000001-2026-4000-a000-000000000310",driverUuid:"cf000002-2026-4000-a000-000000000310",pipelineCacheUuid:"cf000003-2026-4000-a000-000000000310",hwVariableRateShading:!0,hwPushDescriptors:!0,hwMaintenance5:!0,hwMaintenance6:!0,gpuKernelAvailable:!0,gpuKernelLabel:"Universal GPU Devfreq Surface (Preview)",gpuGovernor:"msm-adreno-tz / simple_ondemand",gpuCurrentFreqMhz:680,gpuMaxFreqMhz:1050,adrenoboostLevel:1}),timestamp:Date.now()}}function jy(){const o=new Date,c=m=>new Date(o.getTime()-m*1e3).toTimeString().slice(0,8);return[{id:"preview-1",timestamp:c(9),type:"INFO",tag:"AdmissionGate",message:"Preview: capability semantics, range bounds, and OEM ownership verified."},{id:"preview-2",timestamp:c(6),type:"BASELINE",tag:"Coordinator",message:"Preview: write-ahead transaction journal committed prior to mutation."},{id:"preview-3",timestamp:c(3),type:"INFO",tag:"AutonomousTuner",message:"Preview: sda queue/scheduler switched none -> mq-deadline (read_expire 0.5x, fifo_batch 0.5x) | delta_u=+7.4 -> IMPROVED."}]}function Ty(o,c,m){if(!o.engineActive)return{next:{...o,uclampMin:0,uclampMax:1024,mode:"balanced",modeLabel:"Engine Standby",modeDescription:"Coordinated mutations paused; OEM baseline values intact.",transactionStage:"PLANNED",timestamp:Date.now()}};const d=Number.isFinite(c.thermalTripCelsius)?c.thermalTripCelsius:ts.thermalTripCelsius,q=Number.isFinite(c.balancedUclampMin)?c.balancedUclampMin:ts.balancedUclampMin,k=Math.max(0,Math.min(1024,q)),M=Number.isFinite(o.socTempCelsius)?o.socTempCelsius:38.6,O=Math.max(32,Math.min(49,M+(Math.random()-.46)*.35)),T=m==="cpu"?14:Math.max(0,Math.random()*10),C=m==="memory"?6.4:Math.max(0,Math.random()*3.5),y=m==="io_trial"?11.8:m==="memory"?6.2:Math.max(0,Math.random()*5.2);let z="balanced",Y=k,L=1024;O>=d?(z="thermal_guard",Y=0,L=896):C>=3||y>=5?(z="pressure_guard",Y=Math.floor(k/4)):T>=8&&O<=d-3&&(z="sustained_load",Y=Math.min(1024,k+Math.floor(k/2)));const I=uu(z),J=Math.min(130,Math.max(25,(O-20)*2.2+T*.6+y*.4)),K=Number(((J-(o.aiPredictedPressure10s??45))*.18).toFixed(2)),ae=z==="thermal_guard"?.68:z==="pressure_guard"?.82:Number(Math.max(.72,Math.min(1,1-Math.max(0,J-55)*.004)).toFixed(2)),Z=(o.aiSamples??12)+1,_e=o.blockQueues??[],Se=(o.tunerCandidates??[]).map(Q=>({...Q}));let V=o.tunerStage??"RETAINED",H=o.tunerIntent??"HoldEquilibrium",ne=o.tunerCandidateId??"baseline",De=o.tunerCandidateLabel??"OEM Verified Baseline",Be=o.tunerRationale??"",Me=o.tunerVerdict??"IMPROVED",we=o.tunerUtilityDelta??0,Je=o.tunerTrials??3,it=o.tunerImproved??2;const Ve=o.tunerRolledBack??1,D=_e.map(Q=>({...Q}));if(m==="io_trial"||y>=5&&m!=="memory"){V="RETAINED",H="IoLatencyRelief",ne="io.mq_deadline_low_latency",De="mq-deadline + Low-Latency Read Expiry (0.5x OEM baseline / batch 0.5x)",Me="IMPROVED",we=Number((5.2+Math.random()*4.5).toFixed(2)),Je+=1,it+=1,Be=`I/O PSI=${y.toFixed(1)}% exceeded threshold; switched sda scheduler none -> mq-deadline (read_expire=0.5x, writes_starved=1.5x, fifo_batch=0.5x, read_ahead_kb=2.0x) | Before/After ΔU=+${we} -> RETAINED.`,D.length>0&&(D[0].activeScheduler="mq-deadline",D[0].readExpireMs=250,D[0].writesStarved=3,D[0].fifoBatch=8,D[0].readAheadKb=256,D[0].rqAffinity=2);const Q=Se.find(ke=>ke.id==="io.mq_deadline_low_latency");Q&&(Q.trials+=1,Q.successes+=1,Q.meanDelta=Number(((Q.meanDelta*(Q.trials-1)+we)/Q.trials).toFixed(2)),Q.ucbScore=Number((Q.meanDelta+.65).toFixed(2)))}else m==="memory"||C>=3.2?(V="RETAINED",H="MemoryStallRelief",ne="vm.zram_latency_relief",De="VM Page-Cluster Zero-Latency & VFS Dentry Preservation",Me="IMPROVED",we=Number((3.4+Math.random()*2.8).toFixed(2)),Je+=1,it+=1,Be=`Memory PSI=${C.toFixed(1)}% elevated; applied vm.page-cluster=0 & vfs_cache_pressure=80 | Before/After ΔU=+${we} -> RETAINED.`):z==="thermal_guard"?(V="OBSERVING",H="ThermalStabilization",ne="thermal.schedutil_damped_pacing",De="Thermal Stabilization Governor Damping (2.0x up / 0.5x down rate-limit)",Me="IMPROVED",we=Number((2.8+Math.random()*1.9).toFixed(2)),Je+=1,it+=1,Be=`Thermal headroom constrained (${O.toFixed(1)}°C); switched objective to ThermalStabilization and damped EAS governor ramp rate (2.0x up / 0.5x down relative to OEM baseline).`):(m==="cpu"||T>=8.5)&&(V="RETAINED",H="InteractiveCpuPacing",ne="cpufreq.schedutil_interactive_response",De="EAS Governor Rate-Limit Pacing (500us up / 10000us down)",Me="IMPROVED",we=Number((4.1+Math.random()*2.5).toFixed(2)),Je+=1,it+=1,Be=`CPU PSI=${T.toFixed(1)}% with thermal headroom; paced EAS schedutil governor rate limits (up=500us, down=10000us) | Before/After ΔU=+${we} -> RETAINED.`);const te={...o,mode:z,modeLabel:I.label,modeDescription:I.description,uclampMin:Y,uclampMax:L,socTempCelsius:Number(O.toFixed(1)),cpuPressurePct:Number(T.toFixed(1)),memoryPressurePct:Number(C.toFixed(1)),ioPressurePct:Number(y.toFixed(1)),transactionStage:I.stage,riskClass:I.risk,lastOutcome:I.outcome,ownershipIntact:!0,aiBackend:o.aiBackend??"embedded-rls-mlp",aiOnnxActive:o.aiOnnxActive??!1,aiPredictedPressure10s:Number(J.toFixed(1)),aiPredictedVelocity:K,aiRecommendedScale:ae,aiConfidence:Number(Math.min(.96,.78+Math.min(Z,50)*.003).toFixed(2)),aiSamples:Z,tunerStage:V,tunerIntent:H,tunerCandidateId:ne,tunerCandidateLabel:De,tunerRationale:Be,tunerVerdict:Me,tunerUtilityDelta:we,tunerTrials:Je,tunerImproved:it,tunerRolledBack:Ve,blockQueues:D,tunerCandidates:Se,timestamp:Date.now()};return z===o.mode&&!m?{next:te}:{next:te,event:Cy(z,I.label,te,m)}}let Zc=0;function Cy(o,c,m,d){return Zc+=1,d==="io_trial"?{id:`preview-${Date.now()}-${Zc}`,timestamp:new Date().toTimeString().slice(0,8),type:"BOOST",tag:"AutonomousTuner",message:m.tunerRationale??"Dynamic scheduler trial completed."}:{id:`preview-${Date.now()}-${Zc}`,timestamp:new Date().toTimeString().slice(0,8),type:o==="thermal_guard"?"THERMAL":o==="sustained_load"?"BOOST":"INFO",tag:"Transaction",message:`Stage=${m.transactionStage} -> ${c} | uclamp=${m.uclampMin}/${m.uclampMax} | cpu=${m.cpuPressurePct}% mem=${m.memoryPressurePct}% io=${m.ioPressurePct}% temp=${m.socTempCelsius}C`}}const Pc=[{key:"PLANNED",label:"Planned"},{key:"APPLIED",label:"Applied"},{key:"VERIFIED",label:"Readback Verified"},{key:"OBSERVING",label:"Observing"},{key:"RETAINED",label:"Retained"}],Oy=({telemetry:o,nativeMode:c=!1,thermalTripCelsius:m})=>{var J;const d=uu(o.mode),S=o.socTempCelsius,q=c&&S<0?"N/A":S.toFixed(1),k=o.tunerStage??o.transactionStage??d.stage,M=o.engineActive?Pc.some(K=>K.key===k)?k:"RETAINED":"PLANNED",O=(J=o.blockQueues)==null?void 0:J[0],T=o.tunerCandidates??[],y=[...o.tunerEvaluatedCandidates??[]].sort((K,ae)=>ae.utilityScore-K.utilityScore||ae.confidence-K.confidence),z=[...T].sort((K,ae)=>ae.ucbScore-K.ucbScore),Y=o.tunerUtilityDelta??0,L=o.uclampPolicyMin??o.uclampMin,I=o.uclampPolicyMax??o.uclampMax;return r.jsxs("div",{className:"m3-card p-6 sm:p-8 relative overflow-hidden",children:[r.jsx("div",{"aria-hidden":"true",className:"pointer-events-none absolute -top-28 -right-24 w-96 h-96 rounded-full bg-[#0b57d0]/10 dark:bg-[#a8c7fa]/10 blur-3xl"}),r.jsxs("div",{className:"relative z-10 flex flex-col lg:flex-row lg:items-start justify-between gap-6 pb-6 border-b border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsxs("div",{className:"space-y-2 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400",children:[r.jsxs("span",{className:"font-semibold text-[#0b57d0] dark:text-[#a8c7fa]",children:["Coordinated Mutation Transaction",c?"":" (sim)"]}),r.jsx("span",{"aria-hidden":"true",children:"·"}),r.jsx("span",{children:c?o.engineActive?"Native Runtime":"Native Standby":d.label}),r.jsx("span",{"aria-hidden":"true",children:"·"}),r.jsxs("span",{className:"font-mono",children:["Risk Class: ",o.riskClass??d.risk]})]}),r.jsx("h2",{className:"text-xl sm:text-2xl font-semibold tracking-tight text-slate-900 dark:text-slate-100 leading-snug",children:c?o.modeDescription:d.description}),r.jsx("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed",children:"Mutations are coordinated by optimization intent, dependency provenance, and risk budget. Every kernel write requires a valid baseline, immediate readback verification, and empirical Before/After outcome validation."})]}),r.jsxs("div",{className:"m3-tonal-block px-4 py-3.5 shrink-0 lg:min-w-[250px]",children:[r.jsx("div",{className:"text-[11px] font-medium text-slate-500 dark:text-slate-400 mb-1",children:"Safety Admission Hierarchy"}),r.jsx("div",{className:"text-xs font-semibold text-slate-800 dark:text-slate-200 tracking-tight",children:"OEM Thermal → Pressure → Load → Balanced"}),r.jsxs("div",{className:"text-[11px] font-mono text-slate-500 dark:text-slate-400 mt-1",children:["Verdict: ",o.tunerVerdict??o.lastOutcome??d.outcome," · Ownership: ",o.ownershipIntact!==!1?"Valid":"Released"]})]})]}),r.jsxs("div",{className:"relative z-10 py-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 mb-3",children:[r.jsxs("span",{className:"text-xs font-medium text-slate-600 dark:text-slate-300 flex items-center gap-1.5",children:[r.jsx(Kh,{className:"w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]"}),"Transaction Lifecycle State",c?"":" (sim)"]}),r.jsxs("span",{className:"text-xs font-mono text-slate-500 dark:text-slate-400",children:["Stage",c?"":" (sim)",": ",o.tunerStage??M]}),c&&o.tunerRuntimeStage&&r.jsxs("span",{className:"text-xs font-mono text-slate-500 dark:text-slate-400",children:["Runtime: ",o.tunerRuntimeStage.replace("STAGE_","")]})]}),r.jsx("div",{className:"grid grid-cols-2 sm:grid-cols-5 gap-2",children:Pc.map((K,ae)=>{const Z=Pc.findIndex(V=>V.key===M),_e=K.key===M,Se=ae<Z;return r.jsx("div",{className:"px-3 py-2 rounded-2xl text-xs transition-all flex items-center justify-between "+(_e?"bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold":Se?"bg-slate-900/[0.04] dark:bg-white/[0.05] text-slate-700 dark:text-slate-300 font-medium":"bg-slate-900/[0.02] dark:bg-white/[0.02] text-slate-400 dark:text-slate-500"),children:r.jsxs("span",{className:"truncate",children:[ae+1,". ",K.label]})},K.key)})})]}),r.jsxs("div",{className:"relative z-10 grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 pt-6",children:[r.jsxs("div",{className:"m3-tonal-block p-4 flex flex-col justify-between pixel-uclamp-card",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2",children:[r.jsx("span",{className:"font-medium",children:"Foreground UClamp · Kernel Readback"}),r.jsx(rp,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]"})]}),r.jsxs("div",{className:"text-2xl font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100",children:[o.uclampSupported?o.uclampMin:"—",r.jsxs("span",{className:"text-xs font-normal text-slate-500 dark:text-slate-400",children:[" / ",o.uclampSupported?o.uclampMax:"—"]})]}),r.jsxs("div",{className:"mt-3 grid grid-cols-2 gap-2 border-t border-slate-900/[0.06] dark:border-white/[0.07] pt-3 text-[11px]",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Configured base"}),r.jsxs("div",{className:"mt-0.5 font-mono font-semibold text-slate-800 dark:text-slate-200",children:[L," / ",I]})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"ABI encoding"}),r.jsx("div",{className:"mt-0.5 font-mono font-semibold text-slate-800 dark:text-slate-200",children:o.uclampEncoding??"unknown"})]})]}),r.jsx("div",{className:"mt-2 truncate text-[10px] font-mono text-slate-500 dark:text-slate-400",title:o.uclampTargetPath??"",children:o.uclampSupported?o.uclampTargetPath||"Verified foreground cgroup":"Foreground cgroup capability unavailable"}),r.jsx("div",{className:"mt-1 text-[10px] leading-relaxed text-slate-500 dark:text-slate-400",children:o.uclampSupported?"Live values are read back from the resolved cgroup. Workload, pressure, and thermal budgets may change them from the configured base.":"No verified writable UClamp target; no mutation is claimed."})]}),r.jsxs("div",{className:"m3-tonal-block p-4 flex flex-col justify-between",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2",children:[r.jsxs("span",{className:"font-medium",children:["SoC Thermal Composite",c?"":" (sim)"]}),r.jsx(su,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]"})]}),r.jsxs("div",{className:"text-2xl font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100",children:[q,r.jsx("span",{className:"text-xs font-normal text-slate-500 dark:text-slate-400",children:" °C"})]}),r.jsx("div",{className:"text-xs text-slate-500 dark:text-slate-400 mt-2 leading-normal",children:c?"OEM policy & sensor fusion":`Preview trip threshold ${m.toFixed(1)}°C`})]}),r.jsxs("div",{className:"m3-tonal-block p-4 flex flex-col justify-between",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2",children:[r.jsxs("span",{className:"font-medium",children:["PSI Resource Pressure",c?"":" (sim)"]}),r.jsx($r,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]"})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-2 font-mono tabular-nums text-xs text-slate-800 dark:text-slate-200 my-1",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[11px] text-slate-400",children:"CPU"}),r.jsx("div",{className:"font-semibold",children:o.cpuPressureValid===!1?"N/A":`${o.cpuPressurePct.toFixed(1)}%`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[11px] text-slate-400",children:"MEM"}),r.jsx("div",{className:"font-semibold",children:o.memoryPressureValid===!1?"N/A":`${o.memoryPressurePct.toFixed(1)}%`})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[11px] text-slate-400",children:"I/O"}),r.jsx("div",{className:"font-semibold",children:o.ioPressureValid===!1?"N/A":`${o.ioPressurePct.toFixed(1)}%`})]})]}),r.jsx("div",{className:"text-xs text-slate-500 dark:text-slate-400 mt-1 leading-normal",children:"Linux `/proc/pressure` avg10"})]}),r.jsxs("div",{className:"m3-tonal-block p-4 flex flex-col justify-between",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2",children:[r.jsxs("span",{className:"font-medium",children:["Durable Journal",c?"":" (sim)"]}),r.jsx(ja,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]"})]}),r.jsx("div",{className:"text-lg font-semibold text-slate-900 dark:text-slate-100",children:o.baselineStatus==="protected"?"Journal Armed":"Baseline Restored"}),r.jsx("div",{className:"text-xs text-slate-500 dark:text-slate-400 mt-2 leading-normal",children:o.baselineNodesSnapshotted>0?`${o.baselineNodesSnapshotted} owned kernel nodes verified`:"Write-ahead & readback verified"})]})]}),r.jsxs("div",{className:"relative z-10 mt-4 m3-tonal-block p-4 sm:p-5 space-y-4",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-3 border-b border-slate-900/[0.06] dark:border-white/[0.06] pb-3.5",children:[r.jsxs("div",{className:"space-y-1 min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs",children:[r.jsx(uy,{className:"w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsxs("span",{className:"font-semibold text-slate-900 dark:text-slate-100",children:["Autonomous Contract-Gated Tuning Evaluator",c?"":" (sim)"]}),r.jsxs("span",{className:"px-2 py-0.5 rounded-full text-[11px] font-mono bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:["Intent",c?"":" (sim)",": ",o.tunerIntent??"HoldEquilibrium"]})]}),r.jsx("p",{className:"text-xs text-slate-600 dark:text-slate-400 leading-relaxed",children:o.tunerRationale??"Evaluates workload-specific candidates only after the kernel contract, access path, valid range, and baseline have been verified. Observation-only, test-only, protected, virtual, and unknown storage nodes remain outside automatic mutation."})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-3 sm:gap-4 shrink-0 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"text-[11px] text-slate-400 font-sans",children:["Empirical ΔU",c?"":" (sim)"]}),r.jsx("div",{className:"font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5",children:Y>=0?`+${Y.toFixed(2)}`:Y.toFixed(2)})]}),r.jsxs("div",{children:[r.jsxs("div",{className:"text-[11px] text-slate-400 font-sans",children:["Retained",c?"":" (sim)"]}),r.jsxs("div",{className:"font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5",children:[o.tunerImproved??0," / ",o.tunerTrials??0]})]}),r.jsxs("div",{children:[r.jsxs("div",{className:"text-[11px] text-slate-400 font-sans",children:["Rolled Back",c?"":" (sim)"]}),r.jsx("div",{className:"font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5",children:o.tunerRolledBack??0})]})]})]}),r.jsxs("div",{className:"grid grid-cols-1 lg:grid-cols-2 gap-4",children:[r.jsxs("div",{className:"rounded-2xl bg-white/70 dark:bg-slate-900/40 p-3.5 border border-slate-900/[0.05] dark:border-white/[0.05] space-y-2.5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs",children:[r.jsxs("span",{className:"font-medium text-slate-700 dark:text-slate-200 flex items-center gap-1.5",children:[r.jsx(es,{className:"w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]"}),"Block Queue Surface",c?"":" (sim)"," (",O?`/sys/block/${O.device}/queue`:"Awaiting block queue probe",")"]}),r.jsxs("span",{className:"font-mono text-[11px] text-slate-500 dark:text-slate-400",children:["Baseline: ",(O==null?void 0:O.baselineScheduler)??"N/A"," → Active:"," ",r.jsx("strong",{className:"text-[#0b57d0] dark:text-[#a8c7fa]",children:(O==null?void 0:O.activeScheduler)??"N/A"})]})]}),r.jsxs("div",{className:"grid grid-cols-3 sm:grid-cols-6 gap-2 pt-1 font-mono tabular-nums text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] text-slate-400 font-sans",children:"read_expire"}),r.jsx("div",{className:"font-semibold text-slate-800 dark:text-slate-200",children:O&&O.readExpireMs>=0?`${O.readExpireMs}ms`:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] text-slate-400 font-sans",children:"writes_starved"}),r.jsx("div",{className:"font-semibold text-slate-800 dark:text-slate-200",children:O&&O.writesStarved>=0?O.writesStarved:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] text-slate-400 font-sans",children:"fifo_batch"}),r.jsx("div",{className:"font-semibold text-slate-800 dark:text-slate-200",children:O&&O.fifoBatch>=0?O.fifoBatch:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] text-slate-400 font-sans",children:"read_ahead"}),r.jsx("div",{className:"font-semibold text-slate-800 dark:text-slate-200",children:O&&O.readAheadKb>=0?`${O.readAheadKb}KB`:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] text-slate-400 font-sans",children:"nr_requests"}),r.jsx("div",{className:"font-semibold text-slate-800 dark:text-slate-200",children:O&&O.nrRequests>=0?O.nrRequests:"N/A"})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] text-slate-400 font-sans",children:"rq_affinity"}),r.jsx("div",{className:"font-semibold text-slate-800 dark:text-slate-200",children:O&&O.rqAffinity>=0?O.rqAffinity:"N/A"})]})]})]}),r.jsxs("div",{className:"rounded-2xl bg-white/70 dark:bg-slate-900/40 p-3.5 border border-slate-900/[0.05] dark:border-white/[0.05] space-y-3 pixel-ranking-card",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs",children:[r.jsx("span",{className:"font-semibold text-slate-800 dark:text-slate-100",children:"Global Adaptive Candidate Ranking"}),r.jsxs("span",{className:"font-mono text-[11px] text-slate-500 dark:text-slate-400",children:["Active: ",o.tunerCandidateId??"none"]})]}),r.jsx("p",{className:"text-[10px] leading-relaxed text-slate-500 dark:text-slate-400",children:"One pooled UCB population across all candidates. Domain is metadata only; workload relevance, empirical evidence, eligibility, and safety determine the decision."}),r.jsx("div",{className:"max-h-[360px] space-y-1.5 overflow-y-auto pr-1",children:y.length>0?y.map((K,ae)=>{const Z=T.find(Se=>Se.id===K.id),_e=K.selected||K.id===o.tunerCandidateId;return r.jsxs("div",{className:"rounded-xl border px-3 py-2.5 "+(_e?"border-[#0b57d0]/30 bg-[#d3e3fd]/55 dark:border-[#a8c7fa]/30 dark:bg-[#0842a0]/20":"border-slate-900/[0.05] bg-white/55 dark:border-white/[0.06] dark:bg-white/[0.025]"),children:[r.jsxs("div",{className:"flex items-start justify-between gap-3",children:[r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-1.5 text-[10px] font-mono",children:[r.jsxs("span",{className:"text-slate-400",children:["#",ae+1]}),r.jsx("span",{className:"text-slate-500 dark:text-slate-400",children:K.domain}),_e&&r.jsx("span",{className:"rounded-full bg-[#0b57d0] px-1.5 py-0.5 text-white dark:bg-[#a8c7fa] dark:text-[#062e6f]",children:"ACTIVE"}),!K.eligible&&r.jsx("span",{className:"text-amber-700 dark:text-amber-300",children:"INELIGIBLE"})]}),r.jsx("div",{className:"mt-1 break-all text-[11px] font-semibold text-slate-800 dark:text-slate-100",children:K.id})]}),r.jsxs("div",{className:"shrink-0 text-right font-mono tabular-nums",children:[r.jsx("div",{className:"text-sm font-semibold text-slate-900 dark:text-slate-100",children:K.utilityScore.toFixed(1)}),r.jsx("div",{className:"text-[9px] text-slate-500 dark:text-slate-400",children:"global utility"})]})]}),r.jsxs("div",{className:"mt-2 flex flex-wrap gap-x-3 gap-y-1 font-mono text-[10px] text-slate-500 dark:text-slate-400",children:[r.jsxs("span",{children:["UCB ",K.empiricalScore.toFixed(3)]}),r.jsxs("span",{children:["confidence ",Math.round(K.confidence*100),"%"]}),Z&&r.jsxs("span",{children:["evidence ",Z.rewardSamples??0]}),Z&&r.jsxs("span",{children:["retained ",Z.successes]}),Z&&r.jsxs("span",{children:["rollback ",Z.rollbacks]}),Z&&r.jsxs("span",{children:["attempts ",Z.trials]})]}),!K.eligible&&K.rejectionReason&&r.jsx("div",{className:"mt-1 break-words text-[10px] text-amber-700 dark:text-amber-300",children:K.rejectionReason})]},K.id)}):z.map((K,ae)=>r.jsxs("div",{className:"flex items-center justify-between gap-3 rounded-xl border border-slate-900/[0.05] px-3 py-2 dark:border-white/[0.06]",children:[r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"text-[10px] font-mono text-slate-500 dark:text-slate-400",children:["#",ae+1," · ",K.domain]}),r.jsx("div",{className:"truncate text-[11px] font-semibold text-slate-800 dark:text-slate-100",children:K.id}),r.jsxs("div",{className:"text-[10px] text-slate-500 dark:text-slate-400",children:["evidence ",K.rewardSamples??0," · retained ",K.successes," · rollback ",K.rollbacks]})]}),r.jsxs("div",{className:"shrink-0 text-right font-mono",children:[r.jsx("div",{className:"text-sm font-semibold text-slate-900 dark:text-slate-100",children:K.ucbScore.toFixed(3)}),r.jsx("div",{className:"text-[9px] text-slate-500 dark:text-slate-400",children:"UCB1"})]})]},K.id))})]})]})]}),r.jsxs("div",{className:"relative z-10 mt-4 m3-tonal-block p-4 sm:p-5 flex flex-col sm:flex-row sm:items-center justify-between gap-4",children:[r.jsxs("div",{className:"space-y-1 min-w-0",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs",children:[r.jsxs("span",{className:"font-semibold text-slate-900 dark:text-slate-100",children:["Neural Horizon Predictor (ONNX / Online RLS-MLP)",c?"":" (sim)"]}),r.jsx("span",{className:"px-2 py-0.5 rounded-full text-[11px] font-mono bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:o.aiBackend??"embedded-rls-mlp"})]}),r.jsxs("p",{className:"text-xs text-slate-500 dark:text-slate-400 leading-relaxed",children:["Forecasts 10-second thermal trajectory via Online Recursive Least Squares + 6×8×2 Micro-MLP with NLMS output-head adaptation (or dynamic ",r.jsx("code",{className:"font-mono text-[11px]",children:"libonnxruntime.so"})," C-API when ",r.jsx("code",{className:"font-mono text-[11px]",children:"equilibrium.onnx"})," is present), pre-pacing capacity strictly within Safety Admission invariants."]})]}),r.jsxs("div",{className:"grid grid-cols-4 gap-3 sm:gap-5 shrink-0 font-mono tabular-nums text-xs border-t sm:border-t-0 pt-3 sm:pt-0 border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"text-[11px] text-slate-400 font-sans",children:["10s Forecast",c?"":" (sim)"]}),r.jsx("div",{className:"font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5",children:(o.aiPredictedPressure10s??45).toFixed(1)})]}),r.jsxs("div",{children:[r.jsxs("div",{className:"text-[11px] text-slate-400 font-sans",children:["Velocity",c?"":" (sim)"]}),r.jsx("div",{className:"font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5",children:(o.aiPredictedVelocity??0)>=0?`+${(o.aiPredictedVelocity??0).toFixed(2)}/s`:`${(o.aiPredictedVelocity??0).toFixed(2)}/s`})]}),r.jsxs("div",{children:[r.jsxs("div",{className:"text-[11px] text-slate-400 font-sans",children:["Scale Hint",c?"":" (sim)"]}),r.jsxs("div",{className:"font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5",children:[Math.round((o.aiRecommendedScale??1)*100),"%"]})]}),r.jsxs("div",{children:[r.jsxs("div",{className:"text-[11px] text-slate-400 font-sans",children:["Confidence",c?"":" (sim)"]}),r.jsxs("div",{className:"font-semibold text-sm text-[#0b57d0] dark:text-[#a8c7fa] mt-0.5",children:[Math.round((o.aiConfidence??.85)*100),"%"]})]})]})]})]})},Pf=({nodesSnapshotted:o,nativeMode:c,baselineStatus:m})=>r.jsxs("section",{className:"m3-card space-y-5 p-5 sm:p-7","aria-label":"Baseline ownership and recovery",children:[r.jsxs("div",{className:"flex items-start gap-3",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:r.jsx(ja,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0 flex-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h3",{className:"text-base font-semibold text-slate-900 dark:text-slate-100",children:"Ownership Guard & Baseline Recovery"}),r.jsx("span",{className:"rounded-full px-2.5 py-1 text-[10px] font-bold "+(c?"bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-300":"bg-amber-100 text-amber-800 dark:bg-amber-950/60 dark:text-amber-300"),children:c?"NATIVE JOURNAL":"PREVIEW"})]}),r.jsx("p",{className:"mt-1 text-xs leading-relaxed text-slate-600 dark:text-slate-400",children:"CoreFlow records factory and prior values before mutation, then restores only values it can still prove it owns. Recovery is driven by the daemon journal, not by a dashboard tuning control."})]})]}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx(Jc,{className:"mb-3 h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Journal format"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-semibold text-slate-900 dark:text-slate-100",children:"CFJ4 · W/A"})]}),r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx(ou,{className:"mb-3 h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Ownership state"}),r.jsx("div",{className:"mt-1 text-sm font-semibold text-slate-900 dark:text-slate-100",children:m==="protected"?"Recovery protected":"No pending recovery"})]}),r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx(np,{className:"mb-3 h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Verified snapshots"}),r.jsx("div",{className:"mt-1 font-mono text-sm font-semibold text-slate-900 dark:text-slate-100",children:c?o:`${o} (sim)`})]})]}),r.jsx("p",{className:"border-t border-slate-900/[0.06] pt-4 text-[11px] leading-relaxed text-slate-500 dark:border-white/[0.07] dark:text-slate-400",children:"Manual policy mutation is intentionally absent. If a safety recovery is required, the native recovery lifecycle and module-level kill switch remain authoritative."})]}),Ay={SCREEN_OFF:"Screen Off / Deep Idle",BALANCED_SWEET_SPOT:"Balanced Sweet-Spot",ACTIVE_LOAD:"Active Load / Gaming",THERMAL_GUARD:"Thermal Guard",READ_ONLY:"Read-only status",DISCOVERY:"Capability discovery"},Ff=o=>{if(!Number.isFinite(o)||o<=0)return"Unavailable";const c=["B","KB","MB","GB","TB"];let m=o,d=0;for(;m>=1024&&d<c.length-1;)m/=1024,d+=1;return m.toFixed(d>=3?2:0)+" "+c[d]},qy=o=>{const c=Number(o);return Number.isFinite(c)&&c>=1e5?String(Math.round(c/1e3))+" MHz":o},Ry=({status:o,nativeMode:c,journalOutput:m,restoringBaseline:d,onViewJournal:S,onRestoreBaseline:q})=>{if(!c)return r.jsxs("section",{className:"m3-card space-y-3 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(xl,{className:"h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("h2",{className:"font-semibold",children:"Direct Adaptive runtime"})]}),r.jsx("p",{className:"text-sm text-slate-600 dark:text-slate-400",children:"Live topology, actuator ownership and OEM baseline values are available only from the native daemon. Browser preview does not synthesize kernel audit evidence."})]});if(!o||!o.active)return r.jsxs("section",{className:"m3-card space-y-3 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(xi,{className:"h-4 w-4 text-amber-600 dark:text-amber-300"}),r.jsx("h2",{className:"font-semibold",children:"Direct Adaptive runtime"})]}),r.jsx("p",{className:"text-sm text-slate-600 dark:text-slate-400",children:(o==null?void 0:o.mode)==="READ_ONLY"?"Daemon is stopped. The last OEM baseline remains available for recovery.":"Runtime discovery or durable baseline preparation is unavailable. Independent daemon status remains accessible."})]});const k=o.appliedValues.filter(T=>T.owned).length,M=o.appliedValues.filter(T=>T.modified).length,O=o.ownershipLosses>0||o.appliedValues.some(T=>!T.owned&&T.modified);return r.jsxs("section",{className:"m3-card space-y-5 p-5 sm:p-7","aria-label":"Direct Adaptive runtime and actuator audit",children:[r.jsxs("div",{className:"flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between",children:[r.jsxs("div",{children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(xl,{className:"h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("h2",{className:"font-semibold",children:"Direct Adaptive · Current runtime"})]}),r.jsx("p",{className:"mt-1 text-sm text-slate-600 dark:text-slate-400",children:"Topology-derived policy and readback-verified sysfs ownership. Optional PSI/FrameTimeline evidence does not gate this runtime."})]}),r.jsxs("span",{className:"inline-flex w-fit items-center gap-2 rounded-full px-3 py-1.5 text-xs font-semibold "+(o.journalFault?"bg-rose-100 text-rose-800 dark:bg-rose-950/60 dark:text-rose-200":o.thermalGuard?"bg-amber-100 text-amber-900 dark:bg-amber-950/60 dark:text-amber-200":"bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-200"),children:[o.journalFault?r.jsx(xi,{className:"h-3.5 w-3.5"}):o.thermalGuard?r.jsx($r,{className:"h-3.5 w-3.5"}):r.jsx(ja,{className:"h-3.5 w-3.5"}),o.journalFault?"Journal fault · mutations halted":Ay[o.mode]??o.mode]})]}),r.jsxs("div",{className:"grid grid-cols-2 gap-3 sm:grid-cols-4",children:[r.jsxs("div",{className:"m3-tonal-block p-3",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400",children:[r.jsx(wi,{className:"h-3.5 w-3.5"})," CPU utilization"]}),r.jsxs("div",{className:"mt-1 font-mono text-lg font-semibold tabular-nums",children:[o.cpuUtilizationPct.toFixed(1),"%"]})]}),r.jsxs("div",{className:"m3-tonal-block p-3",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400",children:[r.jsx(es,{className:"h-3.5 w-3.5"})," RAM / ZRAM"]}),r.jsx("div",{className:"mt-1 font-mono text-sm font-semibold",children:Ff(o.ramBytes)}),r.jsxs("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:["ZRAM ",Ff(o.zramBytes)]})]}),r.jsxs("div",{className:"m3-tonal-block p-3",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400",children:[r.jsx(Jh,{className:"h-3.5 w-3.5"})," Display"]}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",children:o.refreshHz>0?String(o.refreshHz)+" Hz":"Auto"}),r.jsxs("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:["Screen ",o.screenState==="OFF"?"off":"on / unknown"]})]}),r.jsxs("div",{className:"m3-tonal-block p-3",children:[r.jsxs("div",{className:"flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400",children:[r.jsx($r,{className:"h-3.5 w-3.5"})," Actuator audit"]}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",children:o.verifiedMutations}),r.jsxs("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:[o.skippedNodes," skipped · ",o.ownershipLosses," ownership losses"]})]})]}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx("div",{className:"text-[11px] font-semibold uppercase tracking-wide text-slate-500 dark:text-slate-400",children:"GPU discovered"}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-semibold",children:o.gpuModel})]}),r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx("div",{className:"text-[11px] font-semibold uppercase tracking-wide text-slate-500 dark:text-slate-400",children:"Storage topology"}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-semibold",children:o.storageType})]})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2",children:[r.jsx("h3",{className:"text-sm font-semibold",children:"CPU topology & discovered OPP"}),r.jsxs("span",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:[o.cpuClusters.length," clusters · ",o.discoveredNodes," nodes"]})]}),o.cpuClusters.length===0?r.jsx("p",{className:"m3-tonal-block p-3 text-xs text-slate-500 dark:text-slate-400",children:"CPUFreq OPP table unavailable; CPU actuation is skipped independently."}):r.jsx("div",{className:"grid grid-cols-1 gap-3 md:grid-cols-2",children:o.cpuClusters.map(T=>r.jsxs("div",{className:"m3-tonal-block min-w-0 p-3",children:[r.jsx("div",{className:"text-xs font-semibold",children:T.id}),r.jsx("div",{className:"mt-2 flex flex-wrap gap-1.5",children:T.opps.map(C=>r.jsx("span",{className:"rounded-md bg-white/70 px-2 py-1 font-mono text-[10px] text-slate-700 dark:bg-white/[0.06] dark:text-slate-200",children:qy(C)},T.id+"-"+C))})]},T.id))})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsxs("div",{className:"flex flex-col gap-1 sm:flex-row sm:items-end sm:justify-between",children:[r.jsx("h3",{className:"text-sm font-semibold",children:"Current applied values vs OEM baseline"}),r.jsxs("span",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:[M," modified · ",k,"/",o.appliedValues.length," owned"]})]}),o.appliedValues.length===0?r.jsx("p",{className:"m3-tonal-block p-3 text-xs text-slate-500 dark:text-slate-400",children:"No writable semantic control nodes were captured."}):r.jsx("div",{className:"space-y-2",children:o.appliedValues.slice(0,24).map(T=>r.jsxs("div",{className:"m3-tonal-block space-y-2 p-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"break-all font-mono text-xs font-semibold",children:T.semantic}),r.jsx("div",{className:"break-all text-[10px] text-slate-500 dark:text-slate-400",children:T.domain})]}),r.jsxs("span",{className:"inline-flex items-center gap-1 rounded-full px-2 py-1 text-[10px] font-semibold "+(T.owned?"bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-200":"bg-amber-100 text-amber-900 dark:bg-amber-950/60 dark:text-amber-200"),children:[T.owned?r.jsx(lu,{className:"h-3 w-3"}):r.jsx(xi,{className:"h-3 w-3"}),T.owned?"Owned / readback OK":T.health]})]}),r.jsxs("div",{className:"grid grid-cols-3 gap-2 text-[10px]",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Current"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-semibold",children:T.current})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"OEM"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-semibold",children:T.oemBaseline})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Policy target"}),r.jsx("div",{className:"mt-0.5 break-all font-mono font-semibold",children:T.target})]})]})]},T.path))})]}),O&&r.jsx("div",{role:"status",className:"rounded-xl border border-amber-300/70 bg-amber-50 px-3 py-2 text-xs text-amber-900 dark:border-amber-800 dark:bg-amber-950/40 dark:text-amber-200",children:"One or more nodes lost mutation ownership or readback agreement. Those actuators are isolated; unrelated domains continue."}),r.jsxs("div",{className:"flex flex-wrap gap-2",children:[r.jsxs("button",{type:"button",onClick:S,className:"inline-flex items-center gap-2 rounded-full bg-[#d3e3fd] px-4 py-2.5 text-xs font-semibold text-[#041e49] hover:bg-[#b8d3f7] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:[r.jsx(Vh,{className:"h-3.5 w-3.5"})," View mutation journal"]}),r.jsxs("button",{type:"button",onClick:q,disabled:d,className:"inline-flex items-center gap-2 rounded-full border border-rose-300/70 px-4 py-2.5 text-xs font-semibold text-rose-800 hover:bg-rose-50 disabled:opacity-50 dark:border-rose-800 dark:text-rose-200 dark:hover:bg-rose-950/30",children:[r.jsx(ap,{className:"h-3.5 w-3.5"})," ",d?"Restoring OEM baseline…":"Restore OEM baseline & stop"]})]}),m!==null&&r.jsxs("div",{className:"space-y-2",children:[r.jsx("div",{className:"text-xs font-semibold",children:"Baseline journal · read-only"}),r.jsx("pre",{className:"max-h-64 overflow-auto rounded-xl bg-slate-950 p-3 text-[10px] leading-relaxed text-emerald-200",children:m})]}),r.jsxs("div",{className:"text-[10px] leading-relaxed text-slate-500 dark:text-slate-400",children:["OEM snapshot entries: ",o.baselineNodes," · thermal pressure: ",o.thermalPressure.toFixed(1)," · journal state: ",o.journalFault?"fault-latched":"healthy"]})]})},Fr=({logs:o,nativeMode:c=!1})=>{const[m,d]=P.useState("ALL"),[S,q]=P.useState(""),[k,M]=P.useState(!1),O=P.useRef(null);P.useEffect(()=>()=>{O.current!==null&&window.clearTimeout(O.current)},[]);const T=P.useMemo(()=>o.filter(y=>{const z=m==="ALL"||y.type===m,Y=!S||y.message.toLowerCase().includes(S.toLowerCase())||y.tag.toLowerCase().includes(S.toLowerCase());return z&&Y}),[o,m,S]),C=async()=>{const y=T.map(z=>`[${z.timestamp}] [${z.type}] ${z.tag}: ${z.message}`).join(`
`);try{await navigator.clipboard.writeText(y),M(!0),O.current!==null&&window.clearTimeout(O.current),O.current=window.setTimeout(()=>{M(!1),O.current=null},2e3)}catch{}};return r.jsxs("div",{className:"m3-card p-6 sm:p-8",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-center justify-between gap-4 pb-5 mb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsxs("div",{className:"flex items-start gap-3.5",children:[r.jsx("div",{className:"w-10 h-10 rounded-2xl bg-[#d3e3fd] dark:bg-[#0842a0] text-[#041e49] dark:text-[#d3e3fd] flex items-center justify-center shrink-0",children:r.jsx(cu,{className:"w-4 h-4"})}),r.jsxs("div",{className:"space-y-1",children:[r.jsx("h3",{className:"font-semibold text-base sm:text-lg text-slate-900 dark:text-slate-100",children:"Observability & Audit Trail"}),r.jsx("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400",children:c?"Live daemon transaction audit stream from runtime stdout/stderr.":"Preview transaction trace showing admission, readback verification, and outcome learning."})]})]}),r.jsx("div",{className:"flex items-center gap-2 shrink-0",children:r.jsxs("button",{type:"button",onClick:C,className:"inline-flex items-center gap-2 px-4 py-2 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-xs font-medium text-slate-700 dark:text-slate-200",children:[k?r.jsx(iu,{className:"w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]"}):r.jsx(ru,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:k?"Copied":"Copy Audit Log"})]})})]}),r.jsxs("div",{className:"flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-3 mb-4",children:[r.jsx("div",{className:"flex flex-wrap items-center gap-1.5 text-xs",children:[{id:"ALL",label:"All Events"},{id:"BOOST",label:"Sustained Load"},{id:"THERMAL",label:"Thermal Gate"},{id:"BASELINE",label:"Journal & Recovery"}].map(y=>{const z=m===y.id;return r.jsx("button",{type:"button","aria-pressed":z,onClick:()=>d(y.id),className:"px-3.5 py-1.5 rounded-full font-medium transition-all "+(z?"bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold":"bg-slate-900/[0.04] hover:bg-slate-900/[0.08] dark:bg-white/[0.05] dark:hover:bg-white/[0.09] text-slate-600 dark:text-slate-400"),children:y.label},y.id)})}),r.jsxs("div",{className:"relative",children:[r.jsx(ip,{className:"w-3.5 h-3.5 text-slate-400 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none"}),r.jsx("input",{type:"text","aria-label":"Filter audit trail",placeholder:"Filter audit trail...",value:S,onChange:y=>q(y.target.value),className:"w-full sm:w-56 pl-9 pr-4 py-1.5 text-xs rounded-full bg-slate-900/[0.04] dark:bg-white/[0.05] border border-slate-900/[0.06] dark:border-white/[0.06] text-slate-800 dark:text-slate-200 focus:outline-none focus:border-[#0b57d0] dark:focus:border-[#a8c7fa]"})]})]}),r.jsx("div",{role:"log","aria-label":"Audit trail",className:"m3-tonal-block p-4 max-h-72 overflow-y-auto font-mono text-xs divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]",children:T.length===0?r.jsx("div",{className:"text-center py-8 text-slate-500 dark:text-slate-400 font-sans text-xs",children:"No audit events match the current filter."}):T.map(y=>r.jsxs("div",{className:"grid grid-cols-1 sm:grid-cols-[76px_88px_110px_1fr] items-baseline gap-2 py-2 px-2 hover:bg-slate-900/[0.03] dark:hover:bg-white/[0.03] rounded-xl transition-colors",children:[r.jsx("span",{className:"text-slate-400 dark:text-slate-500 tabular-nums",children:y.timestamp}),r.jsx("span",{className:"text-[11px] font-semibold text-[#0b57d0] dark:text-[#a8c7fa]",children:y.type}),r.jsx("span",{className:"text-slate-700 dark:text-slate-300 font-medium truncate",children:y.tag}),r.jsx("span",{className:"text-slate-600 dark:text-slate-300 break-words leading-relaxed",children:y.message})]},y.id))})]})},zy=({isDark:o,onToggleTheme:c,modeLabel:m,nativeMode:d=!1,nativeConnected:S=!1,onOpenArchitectureStudio:q})=>r.jsx("header",{className:"pixel-header sticky top-0 z-30",children:r.jsxs("div",{className:"pixel-header-inner mx-auto flex min-h-[4.25rem] max-w-7xl items-center justify-between gap-3 px-4 py-2.5 sm:px-6",children:[r.jsxs("div",{className:"flex min-w-0 flex-1 items-center gap-3",children:[r.jsx("div",{className:"pixel-brandmark flex h-10 w-10 shrink-0 items-center justify-center rounded-2xl",children:r.jsx(wi,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsxs("div",{className:"flex min-w-0 flex-wrap items-center gap-2",children:[r.jsx("span",{className:"truncate text-sm font-bold tracking-tight text-slate-900 dark:text-slate-100 sm:text-base",children:"CFAE System Framework"}),r.jsx("span",{className:"rounded-md bg-slate-900/[0.05] px-1.5 py-0.5 font-mono text-[10px] text-slate-600 dark:bg-white/[0.07] dark:text-slate-300",children:"v4.0.0"}),r.jsxs("span",{className:"pixel-connection inline-flex items-center gap-1.5 rounded-full px-2 py-1 text-[10px] font-bold "+(d&&S?"is-connected":"is-pending"),children:[r.jsx("span",{className:"h-1.5 w-1.5 rounded-full "+(d&&S?"bg-emerald-500":"bg-amber-500")}),d?S?"DAEMON CONNECTED":"AWAITING DATA":"PREVIEW MODE"]})]}),r.jsxs("p",{className:"mt-0.5 truncate text-[11px] text-slate-500 dark:text-slate-400",children:[r.jsx("span",{className:"font-medium text-slate-700 dark:text-slate-300",children:m}),r.jsx("span",{children:" · autonomous system optimization"})]})]})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2",children:[r.jsxs("span",{className:"pixel-autonomous hidden items-center gap-1.5 rounded-full px-3 py-2 text-[11px] font-medium sm:inline-flex",children:[r.jsx(ja,{className:"h-3.5 w-3.5 text-emerald-600 dark:text-emerald-400"}),"Autonomous"]}),r.jsxs("button",{onClick:q,title:"Open Architecture Studio","aria-label":"Open Architecture Studio",className:"pixel-studio inline-flex h-9 items-center justify-center gap-2 rounded-full px-3 text-xs font-semibold",children:[r.jsx(tp,{className:"h-4 w-4"}),r.jsx("span",{className:"hidden sm:inline",children:"Studio"})]}),r.jsx("button",{onClick:c,className:"pixel-theme-toggle flex h-9 w-9 items-center justify-center rounded-full","aria-label":"Toggle color theme",children:o?r.jsx(_y,{className:"h-4 w-4"}):r.jsx($h,{className:"h-4 w-4"})})]})]})}),Dy="# CFAE System Framework v3.1.0 — C++23 + Rust 2024 + Kotlin Equilibrium & Mutation Safety Blueprint\n\n## 1. Runtime Architecture & Language Implementation Stack\n\n- **ISO C++23 Core (`src/daemon/`, `src/kernel/`, `include/coreflow/`)**: Handles semantic sysfs/cgroup v2 discovery, `ResourcePermissionClass` classification (`ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, `IncompleteMetadata`), `CentralMutationCoordinator` domain arbitration, Linux `epoll`/`timerfd`/`signalfd` event loop, `O_NOFOLLOW` + `fstat` (`dev:ino`) object identity checks, durable write-ahead journal (`CFJ4`), persistent bandit state (`CFTS2`), readback verification, and Android service capability probing.\n- **Rust 2024 Decision Engine (`rust/src/`)**: Owns the memory-safe thermal PID controller, hysteresis state machine, PSI pressure arbitration, invalid-sample rejection, and slew-bounded capacity budget via `cxx` FFI.\n- **Kotlin 2.1 Android Integration (`android/src/main/kotlin/io/coreflow/`)**: Provides native Android 15 (API 35) Quick Settings TileService (`CoreFlowTileService`) and thermal broadcast integration (`CoreFlowThermalReceiver`).\n- **Material Design 3 WebUI (`webui/`, `module/webroot/`)**: Lightweight offline dashboard synchronized between `dist/` and `module/webroot/`.\n\n## 2. Coordinated Mutation Transactions (6-Layer Pipeline)\n\nRather than rigid single-parameter-only rules or blind bulk writes, the primary unit of control is the **Coordinated Mutation Transaction**: one explicit optimization objective, a dependency-verified change plan within a bounded risk budget, domain ownership arbitration via `CentralMutationCoordinator`, per-write readback verification, settling-window + noise-floor validation, and ownership-safe rollback.\n\n1. **Universal Capability Classifier**: Classifies every discovered kernel node into `ReadOnly`, `Writable`, `WritableButUnsafe`, `UnknownSemantic`, `VerifiedConstraints`, or `IncompleteMetadata`. Unknown or unsafe nodes are never mutated.\n2. **Workload & Bottleneck Reasoning Engine**: Combines 11-state workload classification, CPU/Memory/I/O PSI, thermal headroom, and temperature trend into an inspectable `OptimizationObjective`.\n3. **Active Thermal Stabilization & Hysteresis Recovery**: Switches objective to `ThermalStabilization` (`thermal.schedutil_damped_pacing`, `thermal.io_burst_pacing`) under non-emergency thermal load, and transitions through hysteresis-guarded `ThermalRecovery` when cooling.\n4. **Central Mutation Coordinator & Dependency Analyzer**: Enforces single-mutation-per-domain leases, priority preemption (`EmergencySafety > ThermalStabilization > PressureGuard > WorkloadOptimization > BackgroundEfficiency`), and `RequiredAnchor -> RequiredDependent` ordering.\n5. **Safety Admission Gate & `CFJ4` Journal**: Verifies pre-write invariants, commits the `CFJ4` write-ahead journal with `boot_id`/`boot_epoch` and `dev:ino` identity prior to the first write, applies changes in dependency order, and verifies every write via readback.\n6. **Outcome Evaluator & `CFTS2` Persistent Learning**: Evaluates post-settling impact against the domain noise floor (`IMPROVED`, `NEUTRAL`, `REGRESSED`, `INCONCLUSIVE`, `ABORTED`, `OWNERSHIP_LOST`, `RECOVERY_REQUIRED`, `REJECTED`), steps down `adaptive_scale` on rollback, enforces context-aware anti-repeat cooldowns, and persists learning atomically in `CFTS2` format guarded by a capability fingerprint.\n\n## 3. Cross-Device Thermal & Power Policy\n\nRaw CPU/GPU junction, battery, and skin temperatures are distinct diagnostics and never share an arbitrary universal cutoff in native mode. Linux thermal-zone values are converted from the documented millidegree ABI.\n\nProduction decisions prioritize the Android NDK thermal severity and headroom API alongside recognized compute, battery, and skin zones evaluated against their read-only OEM passive/hot/critical trip points and secondary `hwmon` sensors. Missing OEM thermal policy retains the stock baseline and reports unavailable.\n\n## 4. Predictive Controller & Slew Bounding\n\nRust consumes a dimensionless thermal-policy pressure index. Its PID controller includes a bounded integral and pressure velocity, bounding output to `0.4–1.0` with a maximum slew of 2% per elapsed second. CPU/GPU ceilings are quantized to exposed operating points while honoring existing OEM maximums.\n\n## 5. Ownership Guard & Idempotent Recovery\n\nFor every mutation, `BaselineStore` records the baseline value, written value, last verified actual value, object identity (`dev:ino`), and ownership state. Rollback restores baseline values only if object identity matches and no external Android/OEM process has taken over (`OWNERSHIP_LOST` retires ownership without overwriting external changes). Crash recovery on startup is strictly idempotent.\n\n## 6. Hardware Introspection & Preservation\n\n- **CPU/GPU Ceilings**: Discovered from `cpufreq` and `devfreq` frequency tables. Missing tables disable that domain.\n- **Battery Step-Charging**: Uses standard `constant_charge_current_max`, evaluates battery temperature exclusively while charging, and never raises an OEM current ceiling.\n- **Storage I/O Weighting & Relative Queue Scaling**: Uses documented cgroup v2 `io.weight` (`default N` format) and scales block queue tunables (`read_ahead_kb`, `nr_requests`, `iosched/*`) relative to captured OEM baselines.\n- **Truthful Vulkan Pass-Through**: `libVkLayer_coreflow.so` reports only physical GPU ICD capabilities (`safe_hardware`) without spoofing API versions or unsupported features.\n\n",My=`use crate::bridge::ffi::{Decision, Parameters, Snapshot};

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
`,Uy=`use crate::controller::{Controller, new_controller};

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
`,By=`/**
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
`,Vy=`/**
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
`,Ly=`#include "coreflow/frequency_controller.hpp"
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
`,Hy=`#include "coreflow/power_bridge.hpp"
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
`,Gy=["Architecture","Mutation Safety","Risk & Provenance","HAL / Binder","Thermal Simulator","C++23 / Rust 2024","Export"],Yy=o=>{const[c,m]=P.useState("Architecture"),[d,S]=P.useState(75),[q,k]=P.useState(30),[M,O]=P.useState(!1),[T,C]=P.useState(""),[y,z]=P.useState("Autonomous tuner"),Y={"Autonomous tuner":By,"Neural predictor":Vy,"Rust controller":My,"cxx bridge":Uy,"CPU/GPU controller":Ly,"Power HAL probe":Hy},L=P.useMemo(()=>{let V=q,H=0,ne=V,De=1;const Be=[];for(let Me=0;Me<=120;Me++){const we=42.5-V;H=Math.max(-50,Math.min(0,H+we));const Je=Me===0?0:V-ne,it=Math.max(.4,Math.min(1,1+(2.5*we+.15*H-1.2*Je)/50));De+=Math.max(-.02,Math.min(.02,it-De)),ne=V,V+=d/100*De*.5-(V-q)*.025,Be.push({temperature:V,scale:De})}return Be},[d,q]),{isOpen:I,onClose:J}=o;if(P.useEffect(()=>{if(!I)return;const V=H=>{H.key==="Escape"&&J()};return window.addEventListener("keydown",V),()=>window.removeEventListener("keydown",V)},[I,J]),P.useEffect(()=>{if(!M)return;const V=window.setTimeout(()=>O(!1),2e3);return()=>window.clearTimeout(V)},[M]),!I)return null;const K=Dy+`

## CFAE System Framework v4.0.0
## Current Hardware
SoC: ${o.socModel}
Kernel: ${o.kernelVersion}
CPUs: ${o.onlineCpus}
GPU: ${o.gpuType}
`,ae=async()=>{try{await navigator.clipboard.writeText(K),O(!0),C("")}catch{C("Clipboard unavailable. Use Download.")}},Z=V=>{const H=URL.createObjectURL(new Blob([K],{type:"text/plain;charset=utf-8"})),ne=document.createElement("a");ne.href=H,ne.download=`cfae_framework_blueprint.${V}`,document.body.appendChild(ne),ne.click(),ne.remove(),window.setTimeout(()=>URL.revokeObjectURL(H),1e3)},_e=L.map((V,H)=>`${H*5},${200-(V.temperature-20)*4}`).join(" "),Se=L.map((V,H)=>`${H*5},${200-V.scale*160}`).join(" ");return r.jsx("div",{className:"fixed inset-0 z-50 flex items-center justify-center p-3 sm:p-6 bg-slate-950/55 backdrop-blur-2xl m3-stage",children:r.jsxs("div",{role:"dialog","aria-modal":"true","aria-label":"CFAE System Framework Architecture Studio",className:"relative w-full max-w-5xl h-[88vh] flex flex-col m3-card text-slate-900 dark:text-slate-100 shadow-2xl overflow-hidden",children:[r.jsxs("header",{className:"px-6 py-5 border-b border-slate-900/[0.06] dark:border-white/[0.06] flex justify-between items-center gap-3",children:[r.jsxs("div",{children:[r.jsx("h2",{className:"font-semibold text-lg text-slate-900 dark:text-white",children:"CFAE Architecture & Mutation Safety Studio"}),r.jsx("p",{className:"text-xs text-[#0b57d0] dark:text-[#a8c7fa] font-mono",children:"v4.0.0 · C++23 + Rust 2024 + Kotlin · Coordinated Mutation Transactions"})]}),r.jsx("button",{onClick:o.onClose,"aria-label":"Close Studio",className:"w-9 h-9 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.1] dark:bg-white/[0.06] dark:hover:bg-white/[0.12] flex items-center justify-center text-slate-600 dark:text-slate-300",children:r.jsx(lp,{size:18})})]}),r.jsx("nav",{className:"flex flex-wrap gap-1.5 px-6 py-3 border-b border-slate-900/[0.06] dark:border-white/[0.06]","aria-label":"Studio sections",children:Gy.map(V=>r.jsx("button",{onClick:()=>m(V),className:"px-3.5 py-1.5 rounded-full text-xs font-medium transition-all "+(V===c?"bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold":"bg-slate-900/[0.04] dark:bg-white/[0.05] text-slate-600 dark:text-slate-300 hover:bg-slate-900/[0.08]"),children:V},V))}),r.jsxs("div",{className:"flex-1 min-h-0 overflow-auto p-6 sm:p-8 space-y-6 text-sm",children:[c==="Architecture"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",children:[r.jsx(wi,{size:18,className:"text-[#0b57d0] dark:text-[#a8c7fa]"}),"Multi-Language Native Architecture"]}),r.jsx("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed",children:"Sensor & Capability Discovery → State Normalization → Reasoning Engine → Dependency Analyzer → Mutation Planner → Safety Admission Gate → Transaction Coordinator (Apply Order + Ownership Guard) → Readback Verify → Outcome Evaluator → Retain / Rollback → Outcome Learning."})]}),r.jsx("div",{className:"grid sm:grid-cols-2 gap-4",children:[["ISO C++23 Core Runtime","Zero-polling epoll/timerfd/signalfd loop, semantic cgroup v2 & sysfs discovery, O_NOFOLLOW + fstat identity verification, and fsync write-ahead journal."],["Rust 2024 Decision Engine","Memory-safe PID thermal controller, hysteresis state machine, PSI contention arbitration, and bounded capacity scaling via cxx bridge."],["Kotlin 2.1 Android Bridge","Android 15 (API 35) Quick Settings TileService and Battery thermal broadcast integration."],["Material 3 WebUI Workbench","Lightweight offline KernelSU/APatch dashboard displaying live runtime status or labeled preview telemetry."]].map(([V,H])=>r.jsxs("div",{className:"m3-tonal-block p-5 space-y-1.5",children:[r.jsx("h4",{className:"text-xs sm:text-sm font-semibold text-[#0b57d0] dark:text-[#a8c7fa]",children:V}),r.jsx("p",{className:"text-xs text-slate-600 dark:text-slate-400 leading-relaxed",children:H})]},V))}),r.jsxs("div",{className:"m3-tonal-block p-5 font-mono text-xs space-y-1.5 text-slate-700 dark:text-slate-300",children:[r.jsxs("div",{children:["SoC: ",o.socModel]}),r.jsxs("div",{children:["Kernel: ",o.kernelVersion]}),r.jsxs("div",{children:["Online CPUs: ",o.onlineCpus]}),r.jsxs("div",{children:["GPU: ",o.gpuType]})]})]}),c==="Mutation Safety"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",children:[r.jsx(ja,{size:18,className:"text-[#0b57d0] dark:text-[#a8c7fa]"}),"Coordinated Optimization & 5-Layer Mutation Safety"]}),r.jsxs("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed",children:["The primary unit of control is the ",r.jsx("strong",{children:"Mutation Transaction"}),": one explicit optimization goal, a dependency-verified change plan within a bounded risk budget, per-write readback verification, and ownership-safe rollback."]})]}),r.jsx("div",{className:"grid sm:grid-cols-2 lg:grid-cols-3 gap-4",children:[["1. Reasoning Engine","Synthesizes workload observations, CPU/Memory/I/O PSI, thermal headroom, and power state into an inspectable optimization intent."],["2. Dependency Analyzer","Builds a provenance-backed dependency graph (Required-before, Requires, Coupled-with, Conflicts-with, Unknown). Unknown edges are never grouped."],["3. Mutation Planner","Constructs a bounded transaction plan specifying baseline, target, allowed range source, execution order, risk budget, and abort conditions."],["4. Safety Admission Gate","Verifies 10 pre-write invariants: object identity, semantic units, valid baseline, zero ownership conflict, and OEM thermal compliance."],["5. Coordinator & Readback","Serializes conflicting transactions, writes the durable journal prior to mutation, applies changes in dependency order, and verifies readback."],["6. Outcome Learning","Evaluates Stability, Responsiveness, Pressure, Thermal, and Power outcomes (IMPROVED, NEUTRAL, REGRESSED, OWNERSHIP_LOST) with anti-repeat deferral."]].map(([V,H])=>r.jsxs("div",{className:"m3-tonal-block p-4 space-y-1.5",children:[r.jsx("h4",{className:"text-xs font-semibold text-[#0b57d0] dark:text-[#a8c7fa]",children:V}),r.jsx("p",{className:"text-xs text-slate-600 dark:text-slate-400 leading-relaxed",children:H})]},V))}),r.jsxs("div",{className:"m3-tonal-block p-5 text-xs space-y-2",children:[r.jsx("div",{className:"font-semibold text-slate-900 dark:text-slate-100",children:"12 Non-Negotiable Safety Invariants:"}),r.jsx("p",{className:"text-slate-600 dark:text-slate-400 leading-relaxed",children:"1. Zero write without valid baseline · 2. Zero mutation without known semantics & range · 3. Zero grouped mutation without verified relationship · 4. No concurrent conflicting transactions · 5. Mandatory readback after every write · 6. Rollback never overwrites external OEM/Android changes · 7. No blind retries without new evidence · 8. Retention requires valid outcome & ownership · 9. No unverified metric claims · 10. OEM/Android thermal policy remains supreme · 11. Write-ahead journal & idempotent recovery · 12. Fail-safe default is no write."})]})]}),c==="Risk & Provenance"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"space-y-2",children:[r.jsx("h3",{className:"font-semibold text-base",children:"Multi-Dimensional Risk Model & Dependency Provenance"}),r.jsx("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed",children:"CoreFlow evaluates seven distinct risk dimensions rather than a single opaque score: Semantic uncertainty, Platform uncertainty, Interaction risk, Reversibility, System impact, Observability, and Ownership confidence."})]}),r.jsx("div",{className:"grid sm:grid-cols-2 gap-4",children:[["Low Risk Class","Strong semantics, bounded impact, reversible, zero conflict, high observability. Eligible for coupled mutation groups within budget."],["Medium Risk Class","Limited interaction or platform uncertainty with understood dependencies and verified rollback. Applied step-by-step in one transaction."],["High Risk Class","Broad impact, uncertain platform semantics, or weak observability. Strictly isolated or rejected; never grouped with other changes."],["Forbidden Class","Missing baseline, unsafe rollback path, ownership conflict, or violates OEM thermal/power safety policy. Never mutated."]].map(([V,H])=>r.jsxs("div",{className:"m3-tonal-block p-4 space-y-1",children:[r.jsx("div",{className:"text-xs font-semibold text-slate-900 dark:text-slate-100",children:V}),r.jsx("p",{className:"text-xs text-slate-600 dark:text-slate-400 leading-relaxed",children:H})]},V))}),r.jsxs("div",{className:"m3-tonal-block p-5 space-y-2 text-xs",children:[r.jsx("div",{className:"font-semibold text-slate-900 dark:text-slate-100",children:"Transaction State Machine Protocol"}),r.jsx("div",{className:"font-mono text-[#0b57d0] dark:text-[#a8c7fa] leading-relaxed",children:"PROPOSED → ADMISSION_CHECK → PREPARED → APPLYING → READBACK_VERIFY → OBSERVING → COMMITTED → RETAINED"}),r.jsx("p",{className:"text-slate-600 dark:text-slate-400 leading-relaxed",children:"Failed admission transitions to DEFERRED / REJECTED. Readback mismatch or observed regression transitions immediately to ROLLING_BACK under Ownership Guard verification."})]})]}),c==="HAL / Binder"&&r.jsxs(r.Fragment,{children:[r.jsx("h3",{className:"font-semibold text-base",children:"Platform Integration & OEM Boundaries"}),r.jsx("div",{className:"m3-tonal-block overflow-hidden",children:r.jsxs("table",{className:"w-full text-xs text-left",children:[r.jsx("thead",{children:r.jsxs("tr",{className:"border-b border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsx("th",{className:"p-4 font-semibold",children:"Capability"}),r.jsx("th",{className:"p-4 font-semibold",children:"Implementation Status"})]})}),r.jsx("tbody",{className:"divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]",children:[["AIDL Power HAL discovery","Nonblocking libbinder_ndk lookup; capability detection only."],["Qualcomm / MediaTek interception","Unavailable: requires a registered, versioned OEM/AOSP proxy backend."],["Binder device access","Does not expose or reroute other processes’ transactions."],["NPU / APU offload","Unavailable: requires a compatible compute workload and runtime."]].map(([V,H])=>r.jsxs("tr",{children:[r.jsx("td",{className:"p-4 font-medium text-[#0b57d0] dark:text-[#a8c7fa]",children:V}),r.jsx("td",{className:"p-4 text-slate-600 dark:text-slate-400",children:H})]},V))})]})}),r.jsx("p",{className:"text-xs text-slate-500 dark:text-slate-400 leading-relaxed",children:"OEM thermal services remain authoritative. The daemon never replaces Power HAL services, patches Binder, or injects undocumented vendor transactions."})]}),c==="Thermal Simulator"&&r.jsxs(r.Fragment,{children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",children:[r.jsx(xl,{size:18,className:"text-[#0b57d0] dark:text-[#a8c7fa]"}),"Illustrative Thermal Equilibrium Plant"]}),r.jsx("p",{className:"text-xs text-slate-600 dark:text-slate-400",children:"Synthetic first-order thermal plant for previewing PID slew bounding. Separate from native device sensor telemetry."}),r.jsxs("div",{className:"grid sm:grid-cols-2 gap-4",children:[r.jsxs("label",{className:"m3-tonal-block p-4 block text-xs",children:[r.jsxs("span",{className:"font-medium",children:["Workload Intensity: ",d,"%"]}),r.jsx("input",{"aria-label":"Simulated workload",className:"block w-full mt-2",type:"range",min:"10",max:"100",value:d,onChange:V=>S(Number(V.target.value))})]}),r.jsxs("label",{className:"m3-tonal-block p-4 block text-xs",children:[r.jsxs("span",{className:"font-medium",children:["Ambient Temperature: ",q," °C"]}),r.jsx("input",{"aria-label":"Simulated ambient temperature",className:"block w-full mt-2",type:"range",min:"20",max:"40",value:q,onChange:V=>k(Number(V.target.value))})]})]}),r.jsxs("svg",{viewBox:"0 0 620 235",role:"img","aria-label":"Temperature and capacity scale over 120 simulated seconds",className:"w-full m3-tonal-block p-2",children:[r.jsx("line",{x1:"10",y1:"200",x2:"610",y2:"200",stroke:"#64748b",strokeOpacity:"0.4"}),r.jsx("polyline",{points:_e,fill:"none",stroke:"#f59e0b",strokeWidth:"2.2",transform:"translate(10,0)"}),r.jsx("polyline",{points:Se,fill:"none",stroke:"#0b57d0",strokeWidth:"2.2",transform:"translate(10,0)"}),r.jsx("text",{x:"12",y:"222",fill:"#64748b",fontSize:"11",children:"0 s"}),r.jsx("text",{x:"565",y:"222",fill:"#64748b",fontSize:"11",children:"120 s"})]}),r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 text-xs font-mono",children:[r.jsxs("span",{children:["Final: ",L[120].temperature.toFixed(1)," °C ·"," ",(L[120].scale*100).toFixed(0),"% capacity budget"]}),r.jsx("span",{className:"text-slate-500",children:"Amber: Temperature (20–70 °C) · Blue: Capacity Scale (40–100%)"})]})]}),c==="C++23 / Rust 2024"&&r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"flex items-center justify-between gap-3",children:[r.jsxs("h3",{className:"font-semibold text-base flex items-center gap-2",children:[r.jsx(ep,{size:18,className:"text-[#0b57d0] dark:text-[#a8c7fa]"}),"Production Source Inspector"]}),r.jsx("select",{"aria-label":"Source file",value:y,onChange:V=>z(V.target.value),className:"px-4 py-1.5 rounded-full text-xs font-medium bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:Object.keys(Y).map(V=>r.jsx("option",{children:V},V))})]}),r.jsx("pre",{className:"overflow-auto text-xs font-mono p-5 m3-tonal-block whitespace-pre max-h-96 leading-relaxed",children:Y[y]})]}),c==="Export"&&r.jsxs(r.Fragment,{children:[r.jsx("h3",{className:"font-semibold text-base",children:"Specification & Blueprint Export"}),r.jsxs("div",{className:"flex gap-2.5 flex-wrap",children:[r.jsxs("button",{className:"inline-flex items-center gap-2 bg-[#0b57d0] dark:bg-[#a8c7fa] text-white dark:text-[#062e6f] px-5 py-2 rounded-full text-xs font-medium",onClick:()=>void ae(),children:[r.jsx(ru,{size:14}),M?"Copied":"Copy Blueprint"]}),["md","txt"].map(V=>r.jsxs("button",{className:"inline-flex items-center gap-2 bg-slate-900/[0.05] dark:bg-white/[0.06] px-4 py-2 rounded-full text-xs font-medium",onClick:()=>Z(V),children:[r.jsx(Uh,{size:14}),"Download .",V]},V))]}),T&&r.jsx("p",{role:"alert",className:"text-xs text-amber-600 dark:text-amber-400",children:T}),r.jsx("pre",{className:"whitespace-pre-wrap font-mono text-xs text-slate-600 dark:text-slate-300 m3-tonal-block p-5 max-h-96 overflow-auto leading-relaxed",children:K})]})]})]})})},Iy=({telemetry:o,nativeMode:c,thermalTripCelsius:m})=>{const[d,S]=P.useState(null),[q,k]=P.useState(""),M=o.socCompositeCelsius??o.socTempCelsius,O=o.cpuAverageCelsius??-1,T=o.cpuPeakCelsius??-1,C=o.gpuTempCelsius??-1,y=o.batteryTempCelsius??-1,z=o.skinTempCelsius??-1,Y=c?!!o.thermalGuardActive:M>=m,L=Math.min(100,Math.max(0,(M-30)/30*100)),I=o.thermalSensors??[],J=I.filter(Z=>!(d!==null&&Z.category!==d||q&&!Z.type.toLowerCase().includes(q.toLowerCase()))),K=Z=>{switch(Z){case 1:return"Compute · CPU/GPU";case 2:return"Battery Cell";case 3:return"Skin · Chassis";case 4:return"Board · PMIC";default:return"Diagnostic · Unclassified"}},ae=Z=>{const _e=Z.trim();if(_e.startsWith("/sys/class/thermal/thermal_zone"))return`tz${_e.slice(31)}`;if(_e.startsWith("thermal_zone"))return`tz${_e.slice(12)}`;const Se=_e.split("/").filter(Boolean);return(Se[Se.length-1]??_e).replace(/^thermal_zone/,"tz")};return r.jsxs("div",{className:"m3-card p-5 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400",children:[r.jsx(su,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsxs("span",{className:"font-semibold text-slate-900 dark:text-slate-100 text-base sm:text-lg",children:["Multi-Sensor Thermal & OEM Policy Matrix",c?"":" (sim)"]}),r.jsx("span",{"aria-hidden":"true",children:"·"}),r.jsxs("span",{className:"font-medium text-[#0b57d0] dark:text-[#a8c7fa]",children:[Y?"ThermalGuard Active Mitigation":"Thermal Headroom Nominal",c?"":" (sim)"]}),r.jsx("span",{"aria-hidden":"true",children:"·"}),r.jsx("span",{children:c?"Hardware sysfs / hwmon Live":"Preview Model"}),o.thermalDataDegraded&&r.jsx("span",{className:"px-2 py-0.5 rounded-full bg-amber-100 text-amber-900 dark:bg-amber-950/70 dark:text-amber-200 font-medium",children:"Sensor Degraded (Partial Read)"}),o.thermalFallbackActive&&r.jsx("span",{className:"px-2 py-0.5 rounded-full bg-amber-100 text-amber-900 dark:bg-amber-950/70 dark:text-amber-200 font-medium",children:"Generic Fallback Mode"})]}),r.jsx("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed",children:"Prioritizes validated Android thermal status and per-zone OEM trip points over single-sensor heuristics. Separate compute, battery, skin, and diagnostic hwmon domains are evaluated without conflating distinct physical units."})]}),r.jsxs("div",{className:"m3-tonal-block px-4 py-2.5 text-xs font-mono shrink-0 self-start",children:[r.jsx("span",{className:"text-slate-500 dark:text-slate-400",children:"Policy Reference: "}),r.jsx("span",{className:"font-semibold text-slate-900 dark:text-slate-100",children:c?o.thermalFallbackActive?`Fallback (${m.toFixed(1)} °C)`:`OEM / Android (${o.thermalSource??"HAL"})`:`${m.toFixed(1)} °C`})]})]}),r.jsxs("div",{className:"m3-tonal-block p-5",children:[r.jsxs("div",{className:"flex items-center justify-between text-xs text-slate-600 dark:text-slate-300 mb-3",children:[r.jsxs("span",{className:"flex items-center gap-2 font-medium",children:[r.jsx(Wc,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0"}),r.jsxs("span",{children:["Weighted SoC Composite Diagnostic",c?"":" (sim)"]})]}),r.jsxs("span",{className:"font-mono tabular-nums font-semibold text-slate-900 dark:text-slate-100 text-sm shrink-0",children:[M>0?M.toFixed(1):"--.-"," °C"]})]}),r.jsx("div",{className:"w-full h-2.5 rounded-full bg-slate-900/[0.08] dark:bg-white/[0.08] overflow-hidden",children:r.jsx("div",{className:"h-full transition-all duration-500 rounded-full bg-[#0b57d0] dark:bg-[#a8c7fa]",style:{width:`${L}%`}})}),r.jsxs("div",{className:"flex justify-between text-[11px] text-slate-500 dark:text-slate-400 mt-2 font-mono tabular-nums",children:[r.jsx("span",{children:"30.0 °C (Idle)"}),r.jsxs("span",{children:[m.toFixed(1)," °C (Mitigation Gate)"]}),r.jsx("span",{children:"60.0 °C (OEM Ceiling)"})]})]}),r.jsx("div",{className:"grid grid-cols-2 lg:grid-cols-5 gap-3.5",children:[{label:c?"CPU Cluster Avg":"CPU Cluster Avg (sim)",val:O,sub:"Little & Big Cores",Icon:wi},{label:c?"CPU Peak Core":"CPU Peak Core (sim)",val:T,sub:"Hottest Core Zone",Icon:rp},{label:c?"GPU Domain":"GPU Domain (sim)",val:C,sub:"Shader / Devfreq",Icon:tp},{label:c?"Battery Cell":"Battery Cell (sim)",val:y,sub:"Charge Guard Input",Icon:ja},{label:c?"Skin / Surface":"Skin / Surface (sim)",val:z,sub:"Chassis Thermistor",Icon:Wc}].map(({label:Z,val:_e,sub:Se,Icon:V})=>r.jsxs("div",{className:"m3-tonal-block p-4 flex flex-col justify-between col-span-1 last:col-span-2 lg:last:col-span-1 min-w-0",children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 text-xs text-slate-500 dark:text-slate-400 mb-2",children:[r.jsx("span",{className:"font-medium truncate",children:Z}),r.jsx(V,{className:"w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0"})]}),r.jsxs("div",{className:"text-xl font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100",children:[_e>0?_e.toFixed(1):"--.-"," ",r.jsx("span",{className:"text-xs font-normal text-slate-500 dark:text-slate-400",children:"°C"})]}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400 mt-1.5 truncate",children:Se})]},Z))}),I.length>0&&r.jsxs("details",{className:"group m3-tonal-block overflow-hidden",children:[r.jsxs("summary",{className:"flex items-center justify-between gap-3 px-5 py-4 cursor-pointer select-none text-xs font-semibold text-slate-800 dark:text-slate-200 hover:bg-slate-900/[0.03] dark:hover:bg-white/[0.03]",children:[r.jsxs("span",{className:"flex items-center gap-2 min-w-0",children:[r.jsx(Hh,{className:"w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0"}),r.jsxs("span",{className:"truncate",children:["Discovered Hardware Thermal Zones",c?"":" (sim)"," (",I.length," Zones)"]})]}),r.jsx("span",{className:"text-xs font-normal text-slate-400 group-open:rotate-180 transition-transform duration-200 shrink-0",children:"▼"})]}),r.jsxs("div",{className:"p-4 sm:p-5 border-t border-slate-900/[0.06] dark:border-white/[0.06] space-y-4",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-center justify-between gap-3",children:[r.jsx("div",{className:"flex gap-1.5 flex-wrap",children:[{id:null,label:"All Zones"},{id:1,label:"Compute"},{id:2,label:"Battery"},{id:3,label:"Skin"},{id:4,label:"Board"}].map(Z=>{const _e=d===Z.id;return r.jsx("button",{onClick:()=>S(Z.id),className:"px-3.5 py-1.5 text-xs rounded-full font-medium transition-all "+(_e?"bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold":"bg-slate-900/[0.04] dark:bg-white/[0.05] text-slate-600 dark:text-slate-300 hover:bg-slate-900/[0.08]"),children:Z.label},String(Z.id))})}),r.jsxs("div",{className:"relative w-full sm:w-56",children:[r.jsx(ip,{className:"w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 text-slate-400"}),r.jsx("input",{type:"text",placeholder:"Search sensor type...",value:q,onChange:Z=>k(Z.target.value),className:"w-full pl-9 pr-4 py-1.5 text-xs rounded-full bg-white/70 dark:bg-slate-900/60 border border-slate-900/[0.08] dark:border-white/[0.08] text-slate-800 dark:text-slate-200 focus:outline-none focus:border-[#0b57d0]"})]})]}),r.jsx("div",{className:"max-h-72 overflow-y-auto rounded-2xl border border-slate-900/[0.06] dark:border-white/[0.06] divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]",children:J.length===0?r.jsx("div",{className:"py-8 text-center text-xs text-slate-500",children:"No matching hardware thermal zones found."}):J.map(Z=>r.jsxs("div",{title:`${Z.id} (${Z.type})`,className:"flex items-center justify-between gap-3 px-3.5 sm:px-4 py-2.5 text-xs hover:bg-slate-900/[0.02] dark:hover:bg-white/[0.02]",children:[r.jsxs("div",{className:"flex items-center gap-2.5 min-w-0 flex-1",children:[r.jsx("span",{className:"font-mono text-[11px] px-2 py-0.5 rounded-lg bg-slate-900/[0.05] dark:bg-white/[0.07] text-slate-600 dark:text-slate-300 shrink-0 min-w-[44px] text-center",children:ae(Z.id)}),r.jsxs("div",{className:"min-w-0 flex-1",children:[r.jsx("div",{className:"font-medium text-slate-800 dark:text-slate-200 truncate",children:Z.type}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400 truncate sm:hidden",children:K(Z.category)})]})]}),r.jsx("span",{className:"text-slate-500 dark:text-slate-400 hidden sm:inline shrink-0 w-36 truncate",children:K(Z.category)}),r.jsxs("div",{className:"text-right shrink-0 pl-2",children:[r.jsx("div",{className:"font-mono tabular-nums font-semibold text-slate-900 dark:text-slate-100",children:Z.celsius!==null&&Z.celsius>=0?`${Z.celsius.toFixed(1)} °C`:r.jsx("span",{className:"text-slate-400 font-normal",children:"Offline"})}),r.jsx("div",{className:"font-mono tabular-nums text-[10px] text-slate-500 dark:text-slate-400 mt-0.5",children:Z.velocityValid?`${Z.velocityCelsiusPerSec>0?"+":""}${Z.velocityCelsiusPerSec.toFixed(2)} °C/s`:Z.valid?Z.velocitySamples!==void 0?`Trend warming up (${Z.velocitySamples}/2)`:c?"Trend unavailable":"Preview trend":"Trend unavailable"})]})]},Z.id+Z.type))})]})]})]})},Qy=({nativeMode:o,onRefreshTelemetry:c})=>{const[m,d]=P.useState("Select an inspection action above to query coreflowd runtime JSON or recovery journal."),[S,q]=P.useState(!1),[k,M]=P.useState(!1),O=P.useRef(null);P.useEffect(()=>()=>{O.current!==null&&window.clearTimeout(O.current)},[]);const T='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"',C=Y=>{const L=Y.trim(),I=L.split(`
`).map(ae=>ae.trim()).filter(Boolean);for(let ae=I.length-1;ae>=0;ae--)if(I[ae].startsWith("{")&&I[ae].endsWith("}"))try{return JSON.parse(I[ae])}catch{}const J=L.indexOf("{"),K=L.lastIndexOf("}");return J>=0&&K>J?JSON.parse(L.slice(J,K+1).replace(/\r?\n/g,"")):JSON.parse(I[I.length-1]??"{}")},y=async(Y,L)=>{q(!0);try{if(!as()){Y.includes("--status")?d(JSON.stringify({running:!0,pid:24108,uclampMin:320,uclampMax:1024,uclampSupported:!0,cpuPressurePct:1.2,memoryPressurePct:.3,ioPressurePct:.4,baselineNodesSnapshotted:6,socTempCelsius:38.6,socCompositeCelsius:38.6,cpuAverageCelsius:38.2,cpuPeakCelsius:41.5,gpuTempCelsius:36.8,batteryTempCelsius:33.4,skinTempCelsius:35.1,recoveryPending:!1,thermalPolicyAvailable:!0,thermalSource:"multi-sensor-composite",thermalSensor:"SoC-Composite (8 CPU cores, 1 GPU domain)",thermalSeverity:0,thermalPressure:65,aiBackend:"embedded-rls-mlp",aiOnnxActive:!1,aiPredictedPressure10s:46.2,aiPredictedVelocity:.08,aiRecommendedScale:.94,aiConfidence:.86,tunerStage:"RETAINED",tunerIntent:"HoldEquilibrium",tunerCandidateId:"io.mq_deadline_low_latency",tunerVerdict:"IMPROVED"},null,2)):Y.includes("--capabilities")?d(JSON.stringify({socModel:"Preview SoC Profile",kernelVersion:"Linux 6.1-android14",onlineCpus:"0-7",gpuType:"Universal Multi-Vendor GPU",rustEngine:!0,powerHalDetected:!0,aiBackend:"embedded-rls-mlp",onnxRuntimeLoaded:!1,binderInterception:!1,npuOffload:!1,frameTelemetry:!1},null,2)):d(`[${L}]
Status: OK (Preview Sandbox)
Exit code: 0`);return}const I=await bi(Y),J=I.errno!==0?`[exit ${I.errno}] ${I.stderr.trim()||"Command failed"}
`:"";if(!I.stdout.trim())d(J||I.stderr||"Command executed with no output.");else try{const K=C(I.stdout);d(J+JSON.stringify(K,null,2))}catch{d(J+I.stdout)}Y.includes("--status")&&await c()}catch(I){d(`Error: ${String(I)}`)}finally{q(!1)}},z=async()=>{try{await navigator.clipboard.writeText(m)}catch{return}M(!0),O.current!==null&&window.clearTimeout(O.current),O.current=window.setTimeout(()=>{M(!1),O.current=null},2e3)};return r.jsxs("div",{className:"m3-card p-6 sm:p-8 space-y-5",children:[r.jsxs("div",{className:"flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400",children:[r.jsx(cu,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("span",{className:"text-base sm:text-lg font-semibold text-slate-900 dark:text-slate-100",children:"Runtime RPC & Diagnostic Console"}),r.jsx("span",{"aria-hidden":"true",children:"·"}),r.jsx("span",{className:"font-mono",children:"coreflow-api.js"}),r.jsx("span",{"aria-hidden":"true",children:"·"}),r.jsx("span",{children:o?"KernelSU Bridge Active":"Preview Simulator"})]}),r.jsx("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed",children:"Inspect raw JSON telemetry, discovered hardware capabilities, and durable journal state directly from `coreflowd`."})]}),r.jsxs("button",{type:"button",onClick:()=>void z(),className:"px-4 py-2 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-xs font-medium text-slate-700 dark:text-slate-200 inline-flex items-center gap-2 self-start",children:[k?r.jsx(iu,{className:"w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]"}):r.jsx(ru,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:k?"Copied":"Copy Output"})]})]}),r.jsx("div",{className:"flex flex-wrap gap-2",children:[{label:"Query Status (--status)",cmd:`${T}; "$BIN" --status`,title:"Daemon Status",Icon:ty},{label:"Capabilities (--capabilities)",cmd:`${T}; "$BIN" --capabilities`,title:"Hardware Capabilities",Icon:ep},{label:"Inspect Recovery Journal",cmd:`${T}; "$BIN" --journal`,title:"Recovery Journal",Icon:np},{label:"Inspect coreflow.conf",cmd:"cat /data/adb/coreflow/coreflow.conf 2>/dev/null",title:"Configuration File",Icon:xi}].map(({label:Y,cmd:L,title:I,Icon:J})=>r.jsxs("button",{type:"button",onClick:()=>void y(L,I),disabled:S,className:"px-4 py-2 rounded-full bg-[#d3e3fd]/70 hover:bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0]/60 dark:hover:bg-[#0842a0] dark:text-[#d3e3fd] text-xs font-medium transition-all inline-flex items-center gap-2 disabled:opacity-50",children:[r.jsx(J,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:Y})]},I))}),r.jsx("div",{className:"m3-tonal-block p-5 font-mono text-xs text-slate-800 dark:text-slate-200 overflow-x-auto max-h-80",children:r.jsx("pre",{className:"whitespace-pre leading-relaxed",children:m})})]})},Ky='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"; [ -f "$BIN" ] || exit 127',Xy=[{title:"Boot stability",detail:"Daemon readiness, initial hardware discovery and recovery state"},{title:"Idle equilibrium",detail:"Baseline stability, drift detection and release of owned mutations"},{title:"Interactive workload",detail:"Foreground process membership, CPU pressure and response pacing"},{title:"Sustained workload",detail:"Long-running workload admission and thermal response"},{title:"Multitasking",detail:"Memory pressure, I/O pressure and zram activity"},{title:"Thermal recovery",detail:"Cooling trend, OEM thermal constraints and post-load stabilization"},{title:"Ownership conflict",detail:"External writer detection and safe mutation relinquishment"},{title:"Journal recovery",detail:"Write-ahead journal integrity and idempotent recovery admission"}],Zy=({nativeMode:o})=>{const[c,m]=P.useState("unavailable"),[d,S]=P.useState(null),[q,k]=P.useState("Waiting for native validation status."),[M,O]=P.useState(null),[T,C]=P.useState(null),y=P.useCallback(async()=>{if(!o||!as()){m("unavailable"),S(null),k("Native validation status is unavailable in browser preview.");return}try{const Y=await bi(`${Ky}; "$BIN" --dvs-status`);if(Y.errno!==0)throw new Error(Y.stderr||Y.stdout||"Validation status query failed");const L=Y.stdout.trim(),I=L.match(/^OK ACTIVE ([0-9a-f-]{36})$/i);if(I)m("active"),S(I[1]),k("A native validation capture is active. Its lifecycle remains daemon-owned.");else if(L==="OK INACTIVE")m("idle"),S(null),k("No validation capture is currently active.");else throw new Error(L||"Unexpected CF-DVS status response");C(null),O(new Date().toLocaleTimeString())}catch(Y){m("unavailable"),S(null),C(Y instanceof Error?Y.message:String(Y)),k("Unable to read the native validation session state.")}},[o]);P.useEffect(()=>{let Y=!1;const L=async()=>{Y||await y()};L();const I=window.setInterval(()=>void L(),15e3);return()=>{Y=!0,window.clearInterval(I)}},[y]);const z=c==="active"?"Capture active":c==="idle"?"Idle":"Unavailable";return r.jsxs("div",{className:"m3-card space-y-6 p-6 sm:p-8",children:[r.jsxs("div",{className:"flex items-start gap-3 border-b border-slate-900/[0.06] pb-5 dark:border-white/[0.06]",children:[r.jsx("div",{className:"flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:r.jsx(xl,{className:"h-5 w-5"})}),r.jsxs("div",{className:"min-w-0 flex-1",children:[r.jsxs("div",{className:"flex flex-wrap items-center gap-2",children:[r.jsx("h2",{className:"text-base font-semibold text-slate-900 dark:text-slate-100",children:"Device validation coverage"}),r.jsx("span",{className:"rounded-full bg-slate-900/[0.05] px-2.5 py-1 text-[10px] font-bold text-slate-600 dark:bg-white/[0.07] dark:text-slate-300",children:"READ ONLY"})]}),r.jsx("p",{className:"mt-1 text-xs leading-relaxed text-slate-600 dark:text-slate-400",children:"CoreFlow validates its own runtime invariants. This surface explains coverage and reports the native capture state; it does not start, stop, or configure a device test session."})]})]}),r.jsxs("section",{className:"m3-tonal-block flex flex-col gap-3 p-4 sm:flex-row sm:items-center sm:justify-between",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-[10px] font-semibold uppercase tracking-[0.16em] text-slate-500 dark:text-slate-400",children:"Native CF-DVS session"}),r.jsxs("div",{className:"mt-2 flex items-center gap-2 text-sm font-semibold text-slate-900 dark:text-slate-100",children:[r.jsx("span",{className:`h-2.5 w-2.5 rounded-full ${c==="active"?"bg-emerald-500":c==="idle"?"bg-slate-400":"bg-amber-500"}`}),z]}),r.jsx("p",{className:"mt-1 break-all text-xs text-slate-600 dark:text-slate-400",children:d?`Session ${d}`:q}),M&&r.jsxs("p",{className:"mt-1 text-[10px] text-slate-500 dark:text-slate-400",children:["Last checked ",M," · automatic refresh every 15 seconds"]})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-2 text-xs text-slate-500 dark:text-slate-400",children:[r.jsx(ou,{className:"h-4 w-4"})," Daemon-owned lifecycle"]})]}),T&&r.jsxs("div",{role:"status",className:"flex items-start gap-2 rounded-xl bg-amber-50 p-3 text-xs text-amber-900 dark:bg-amber-950/30 dark:text-amber-200",children:[r.jsx(xi,{className:"mt-0.5 h-4 w-4 shrink-0"}),r.jsx("span",{children:T})]}),r.jsxs("section",{className:"space-y-3",children:[r.jsxs("div",{children:[r.jsx("h3",{className:"text-sm font-semibold text-slate-900 dark:text-slate-100",children:"Validation domains"}),r.jsx("p",{className:"mt-1 text-xs text-slate-600 dark:text-slate-400",children:"These are evidence domains, not user-selectable performance profiles."})]}),r.jsx("div",{className:"grid grid-cols-1 gap-2 sm:grid-cols-2",children:Xy.map(Y=>r.jsxs("div",{className:"flex items-start gap-3 rounded-2xl border border-slate-900/[0.06] p-3 dark:border-white/[0.07]",children:[r.jsx(lu,{className:"mt-0.5 h-4 w-4 shrink-0 text-emerald-600 dark:text-emerald-400"}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-xs font-semibold text-slate-800 dark:text-slate-200",children:Y.title}),r.jsx("p",{className:"mt-1 text-[11px] leading-relaxed text-slate-500 dark:text-slate-400",children:Y.detail})]})]},Y.title))})]}),r.jsx("p",{className:"border-t border-slate-900/[0.06] pt-4 text-[11px] leading-relaxed text-slate-500 dark:border-white/[0.07] dark:text-slate-400",children:"Diagnostic capture is not an optimization command. Evidence collection and session finalization are orchestrated by the native runtime lifecycle; the WebUI cannot manually invoke them."})]})},be=({label:o,value:c,note:m,simulated:d=!1})=>{const S=typeof c=="boolean";return r.jsxs("div",{className:"flex items-center justify-between py-2.5 border-b border-slate-900/[0.05] dark:border-white/[0.05] last:border-b-0 text-xs",children:[r.jsxs("div",{className:"text-slate-600 dark:text-slate-300 pr-3",children:[r.jsxs("span",{children:[o,d?" (sim)":""]}),m&&r.jsxs("span",{className:"text-[11px] text-slate-400 dark:text-slate-500",children:[" · ",m]})]}),S?r.jsx("div",{className:"inline-flex items-center gap-1.5 font-semibold font-mono tabular-nums shrink-0 "+(c?"text-emerald-700 dark:text-emerald-400":"text-slate-500 dark:text-slate-400"),children:c?r.jsxs(r.Fragment,{children:[r.jsx(iu,{className:"w-3.5 h-3.5 shrink-0"}),r.jsx("span",{children:"Yes"})]}):r.jsxs(r.Fragment,{children:[r.jsx(lp,{className:"w-3.5 h-3.5 shrink-0 opacity-60"}),r.jsx("span",{children:"No"})]})}):r.jsx("span",{className:"font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100 shrink-0",children:c})]})},Py=({vulkanTelemetry:o,vulkanConfig:c,nativeMode:m,onSaveVulkanConfig:d,isSaving:S,saveMessage:q})=>{const k=gl(c,o),M=(T,C,y=!1)=>{if(T)return C?"HW Native":void 0},O=async T=>{const y={...c,mode:T==="disabled"?"disabled":"safe_hardware"};await d(y)};return r.jsxs("div",{className:"space-y-6",children:[r.jsxs("div",{className:"rounded-[28px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 sm:p-8 space-y-6",children:[r.jsxs("div",{className:"flex flex-col lg:flex-row lg:items-start lg:justify-between gap-4",children:[r.jsxs("div",{className:"space-y-1.5 max-w-2xl",children:[r.jsxs("div",{className:"text-xs text-slate-500 dark:text-slate-400",children:[r.jsxs("span",{children:["Universal Multi-Vendor GPU & Truthful Vulkan Diagnostics",m?"":" (sim)"]}),r.jsx("span",{"aria-hidden":"true",children:" · "}),r.jsx("span",{children:k.deviceName}),r.jsx("span",{"aria-hidden":"true",children:" · "}),r.jsx("span",{children:m?"Live Device ICD Probe":"Hardware Pass-Through Model"})]}),r.jsxs("h2",{className:"text-xl sm:text-2xl font-bold tracking-tight text-slate-900 dark:text-slate-100",children:["Vulkan Capabilities & Truthful Hardware Pass-Through",m?"":" (sim)"]}),r.jsxs("p",{className:"text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed",children:["Dynamically probes the active device's physical GPU ICD (Qualcomm Adreno KGSL, ARM Mali / Immortalis, Samsung Xclipse, PowerVR) and reports genuine hardware capabilities without fabricating unsupported extensions, feature flags, or API versions. Opt-in diagnostic layer"," ",r.jsx("code",{className:"font-mono text-xs",children:"libVkLayer_coreflow.so"})," requires"," ",r.jsx("code",{className:"font-mono text-xs",children:"ENABLE_COREFLOW_VULKAN_LAYER=1"}),"."]})]}),r.jsxs("div",{className:"text-left lg:text-right font-mono tabular-nums text-xs text-slate-600 dark:text-slate-400 shrink-0 space-y-1",children:[r.jsxs("div",{children:["Hardware API:"," ",r.jsxs("span",{className:"font-semibold text-slate-900 dark:text-slate-100",children:["Vulkan ",k.apiVersion]})]}),r.jsxs("div",{children:["Device Extensions:"," ",r.jsx("span",{className:"font-semibold text-slate-900 dark:text-slate-100",children:k.deviceExtensions})," · ","Instance: ",k.instanceExtensions]})]})]}),r.jsxs("div",{className:"space-y-3",children:[r.jsx("div",{className:"text-xs font-semibold text-slate-700 dark:text-slate-300",children:"Select Vulkan Layer Mode"}),r.jsxs("div",{className:"grid grid-cols-1 md:grid-cols-2 gap-3",children:[r.jsxs("button",{type:"button",disabled:S,"aria-pressed":c.mode==="disabled",onClick:()=>void O("disabled"),className:"text-left p-4 rounded-2xl border transition-colors "+(c.mode==="disabled"?"bg-[#d3e3fd]/45 dark:bg-[#0842a0]/30 border-[#0842a0]/40 dark:border-[#d3e3fd]/40 text-slate-900 dark:text-slate-100":"bg-slate-50/70 dark:bg-[#1b202a] border-slate-900/[0.06] dark:border-white/[0.06] text-slate-700 dark:text-slate-300 hover:bg-slate-100/80 dark:hover:bg-[#202632]"),children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 mb-1",children:[r.jsx("span",{className:"text-xs font-bold whitespace-nowrap",children:"Default · Implicit Layer Disabled (Recommended)"}),r.jsx(wi,{className:"w-4 h-4 shrink-0 text-slate-500"})]}),r.jsxs("p",{className:"text-[11px] text-slate-600 dark:text-slate-400 leading-relaxed",children:["Keeps ",r.jsx("code",{className:"font-mono",children:"VK_LAYER_COREFLOW_universal_bridge"})," ","inactive for normal applications while still reading native GPU devfreq and physical ICD capabilities."]})]}),r.jsxs("button",{type:"button",disabled:S,"aria-pressed":c.mode!=="disabled",onClick:()=>void O("safe_hardware"),className:"text-left p-4 rounded-2xl border transition-colors "+(c.mode!=="disabled"?"bg-[#d3e3fd]/45 dark:bg-[#0842a0]/30 border-[#0842a0]/40 dark:border-[#d3e3fd]/40 text-slate-900 dark:text-slate-100":"bg-slate-50/70 dark:bg-[#1b202a] border-slate-900/[0.06] dark:border-white/[0.06] text-slate-700 dark:text-slate-300 hover:bg-slate-100/80 dark:hover:bg-[#202632]"),children:[r.jsxs("div",{className:"flex items-center justify-between gap-2 mb-1",children:[r.jsx("span",{className:"text-xs font-bold whitespace-nowrap",children:"Truthful Hardware Pass-Through"}),r.jsx(oy,{className:"w-4 h-4 shrink-0 text-sky-600 dark:text-sky-400"})]}),r.jsxs("p",{className:"text-[11px] text-slate-600 dark:text-slate-400 leading-relaxed",children:["Uses per-handle Vulkan dispatch and forwards 100% authentic ICD capabilities without spoofing API versions, injecting extensions, or stripping ",r.jsx("code",{className:"font-mono",children:"vkCreateDevice"})," ","requests."]})]})]})]}),r.jsxs("div",{className:"flex flex-col gap-3 border-t border-slate-900/[0.06] pt-4 dark:border-white/[0.06] sm:flex-row sm:items-center sm:justify-between",children:[r.jsxs("div",{className:"max-w-2xl text-xs leading-relaxed text-slate-600 dark:text-slate-400",children:[r.jsx("p",{className:"font-semibold text-slate-800 dark:text-slate-200",children:"Manual Vulkan diagnostic layer"}),r.jsx("p",{className:"mt-1",children:"This opt-in controls only the Vulkan diagnostic / pass-through layer. It does not enable rendering optimizers or fabricate hardware capabilities. All CPU, memory, storage and thermal policies remain daemon-owned."})]}),r.jsxs("div",{className:"flex shrink-0 items-center gap-3",children:[S?r.jsx("span",{className:"text-xs text-slate-600 dark:text-slate-400",children:"Applying Vulkan mode…"}):q&&r.jsx("span",{className:"text-xs text-slate-600 dark:text-slate-400",children:q}),r.jsx("span",{className:"rounded-full bg-slate-900/[0.05] px-3 py-2 text-[11px] font-semibold text-slate-700 dark:bg-white/[0.07] dark:text-slate-200",children:c.mode==="disabled"?"Layer disabled":"Layer enabled"})]})]})]}),r.jsxs("div",{className:"grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6",children:[r.jsxs("div",{className:"rounded-[24px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 space-y-5",children:[r.jsxs("div",{children:[r.jsx("h3",{className:"text-sm font-bold text-slate-900 dark:text-slate-100 mb-2",children:"Capabilities"}),r.jsxs("div",{className:"divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]",children:[r.jsx(be,{simulated:!m,label:"Ray tracing",value:k.rayTracing,note:M(k.rayTracing,k.hwRayTracing,!0)}),r.jsx(be,{simulated:!m,label:"Mesh shaders",value:k.meshShaders,note:M(k.meshShaders,k.hwMeshShaders,!0)}),r.jsx(be,{simulated:!m,label:"Variable rate shading",value:k.variableRateShading,note:M(k.variableRateShading,k.hwVariableRateShading)}),r.jsx(be,{simulated:!m,label:"Cooperative matrix",value:k.cooperativeMatrix,note:M(k.cooperativeMatrix,k.hwCooperativeMatrix,!0)}),r.jsx(be,{simulated:!m,label:"RT pipeline",value:k.rtPipeline,note:M(k.rtPipeline,k.hwRtPipeline,!0)}),r.jsx(be,{simulated:!m,label:"Ray query",value:k.rayQuery,note:M(k.rayQuery,k.hwRayQuery,!0)}),r.jsx(be,{simulated:!m,label:"Acceleration structure",value:k.accelerationStructure,note:M(k.accelerationStructure,k.hwAccelerationStructure,!0)}),r.jsx(be,{simulated:!m,label:"Portability subset",value:k.portabilitySubset})]})]}),r.jsxs("div",{className:"pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsx("h3",{className:"text-sm font-bold text-slate-900 dark:text-slate-100 mb-2",children:"Core Features"}),r.jsxs("div",{children:[r.jsx(be,{simulated:!m,label:"Robust buffer access",value:k.robustBufferAccess}),r.jsx(be,{simulated:!m,label:"Sampler anisotropy",value:k.samplerAnisotropy}),r.jsx(be,{simulated:!m,label:"Shader Int16",value:k.shaderInt16}),r.jsx(be,{simulated:!m,label:"Shader Float64",value:k.shaderFloat64,note:M(k.shaderFloat64,k.hwShaderFloat64,!0)})]})]})]}),r.jsxs("div",{className:"rounded-[24px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 space-y-5",children:[r.jsxs("div",{children:[r.jsx("h3",{className:"text-sm font-bold text-slate-900 dark:text-slate-100 mb-2",children:"Vulkan 1.1 · 1.2 · 1.3"}),r.jsxs("div",{children:[r.jsx(be,{simulated:!m,label:"Protected memory",value:k.protectedMemory,note:"1.1"}),r.jsx(be,{simulated:!m,label:"Timeline semaphore",value:k.timelineSemaphore,note:"1.2"}),r.jsx(be,{simulated:!m,label:"Descriptor indexing",value:k.descriptorIndexing,note:"1.2"}),r.jsx(be,{simulated:!m,label:"Buffer device address",value:k.bufferDeviceAddress,note:"1.2"}),r.jsx(be,{simulated:!m,label:"Dynamic rendering",value:k.dynamicRendering,note:"1.3"}),r.jsx(be,{simulated:!m,label:"Synchronization2",value:k.synchronization2,note:"1.3"})]})]}),r.jsxs("div",{className:"pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsx("h3",{className:"text-sm font-bold text-slate-900 dark:text-slate-100 mb-2",children:"Vulkan 1.4"}),r.jsxs("div",{children:[r.jsx(be,{simulated:!m,label:"Index type UINT8",value:k.indexTypeUint8,note:M(k.indexTypeUint8,k.hwIndexTypeUint8)}),r.jsx(be,{simulated:!m,label:"Line rasterization",value:k.lineRasterization,note:M(k.lineRasterization,k.hwLineRasterization)}),r.jsx(be,{simulated:!m,label:"Push descriptors",value:k.pushDescriptors,note:M(k.pushDescriptors,k.hwPushDescriptors)}),r.jsx(be,{simulated:!m,label:"Global priority query",value:k.globalPriorityQuery,note:M(k.globalPriorityQuery,k.hwGlobalPriorityQuery)}),r.jsx(be,{simulated:!m,label:"Shader float controls 2",value:k.shaderFloatControls2,note:M(k.shaderFloatControls2,k.hwShaderFloatControls2)}),r.jsx(be,{simulated:!m,label:"Max push descriptors",value:k.maxPushDescriptors})]})]})]}),r.jsxs("div",{className:"rounded-[24px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 space-y-5",children:[r.jsxs("div",{children:[r.jsx("h3",{className:"text-sm font-bold text-slate-900 dark:text-slate-100 mb-2",children:"Highlights"}),r.jsxs("div",{children:[r.jsx(be,{simulated:!m,label:"Maintenance5",value:k.maintenance5,note:M(k.maintenance5,k.hwMaintenance5)}),r.jsx(be,{simulated:!m,label:"Maintenance6",value:k.maintenance6,note:M(k.maintenance6,k.hwMaintenance6)}),r.jsx(be,{simulated:!m,label:"Maintenance7",value:k.maintenance7,note:M(k.maintenance7,k.hwMaintenance7)}),r.jsx(be,{simulated:!m,label:"Descriptor buffer",value:k.descriptorBuffer,note:M(k.descriptorBuffer,k.hwDescriptorBuffer)}),r.jsx(be,{simulated:!m,label:"Graphics pipeline library",value:k.graphicsPipelineLibrary,note:M(k.graphicsPipelineLibrary,k.hwGraphicsPipelineLibrary)})]})]}),r.jsxs("div",{className:"pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]",children:[r.jsx("h3",{className:"text-sm font-bold text-slate-900 dark:text-slate-100 mb-2",children:"Universal Multi-Vendor GPU Surface"}),r.jsxs("div",{children:[r.jsx(be,{simulated:!m,label:"Detected GPU",value:k.deviceName}),r.jsx(be,{simulated:!m,label:"Vendor Domain",value:k.vendor}),r.jsx(be,{simulated:!m,label:"Active Governor",value:k.gpuGovernor}),r.jsx(be,{simulated:!m,label:"Clock Window",value:`${k.gpuCurrentFreqMhz} / ${k.gpuMaxFreqMhz} MHz`}),r.jsx(be,{simulated:!m,label:"Adrenoboost / Pacing",value:`Level ${k.adrenoboostLevel}`})]})]}),r.jsxs("div",{className:"pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06] space-y-2",children:[r.jsx("h3",{className:"text-sm font-bold text-slate-900 dark:text-slate-100",children:"Hardware & Cache UUIDs"}),r.jsxs("div",{className:"space-y-1.5 text-[11px] font-mono tabular-nums text-slate-600 dark:text-slate-400",children:[r.jsxs("div",{className:"flex flex-col",children:[r.jsx("span",{className:"text-slate-400 dark:text-slate-500",children:"Device UUID"}),r.jsx("span",{className:"text-slate-800 dark:text-slate-200 break-all",children:k.deviceUuid})]}),r.jsxs("div",{className:"flex flex-col",children:[r.jsx("span",{className:"text-slate-400 dark:text-slate-500",children:"Driver UUID"}),r.jsx("span",{className:"text-slate-800 dark:text-slate-200 break-all",children:k.driverUuid})]}),r.jsxs("div",{className:"flex flex-col",children:[r.jsx("span",{className:"text-slate-400 dark:text-slate-500",children:"PipelineCache UUID"}),r.jsx("span",{className:"text-slate-800 dark:text-slate-200 break-all",children:k.pipelineCacheUuid})]})]})]})]})]})]})},Fy=o=>o==="validated"?"bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-300":"bg-amber-100 text-amber-800 dark:bg-amber-950/60 dark:text-amber-300",Jy=({evidence:o,nativeConnected:c,simulationMode:m})=>r.jsxs("section",{"aria-label":"Native semantic evidence",className:"m3-card p-5 sm:p-6 space-y-4",children:[r.jsxs("div",{className:"flex flex-wrap items-start justify-between gap-3",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(Jc,{className:"w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsxs("div",{children:[r.jsx("h3",{className:"text-sm font-semibold text-slate-900 dark:text-slate-100",children:"Native Semantic Evidence"}),r.jsx("p",{className:"text-xs text-slate-500 dark:text-slate-400",children:"Daemon-owned CapabilityResolver assessment · schema v1"})]})]}),c?r.jsxs("span",{className:"inline-flex items-center gap-1.5 rounded-full px-2.5 py-1 text-[11px] font-semibold bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-300",children:[r.jsx(lu,{className:"w-3.5 h-3.5"})," Connected"]}):r.jsxs("span",{className:"inline-flex items-center gap-1.5 rounded-full px-2.5 py-1 text-[11px] font-semibold bg-amber-100 text-amber-800 dark:bg-amber-950/60 dark:text-amber-300",children:[r.jsx(jh,{className:"w-3.5 h-3.5"}),m?"Simulation Mode":"Awaiting Official Data"]})]}),m?r.jsxs("div",{role:"note",className:"rounded-xl border border-amber-300/70 bg-amber-50 px-4 py-3 text-xs leading-relaxed text-amber-900 dark:border-amber-800/70 dark:bg-amber-950/30 dark:text-amber-200",children:[r.jsx("strong",{children:"Simulation Mode."})," Preview metrics are synthetic and labelled (sim). No kernel semantic binding or audit evidence is inferred in the browser."]}):(o==null?void 0:o.status)==="unavailable"?r.jsxs("div",{className:"rounded-xl bg-slate-100/80 dark:bg-white/[0.04] px-4 py-3 flex gap-2.5 text-xs text-slate-600 dark:text-slate-300",children:[r.jsx(xi,{className:"w-4 h-4 shrink-0 text-amber-600 dark:text-amber-400"}),r.jsx("span",{children:"The daemon returned schema v1 with status unavailable because the adapter has not produced semantic evidence. No binding state or functional domain is fabricated."})]}):c?o?r.jsxs(r.Fragment,{children:[r.jsxs("div",{className:"grid grid-cols-2 sm:grid-cols-4 gap-2 text-xs",children:[r.jsxs("div",{className:"rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3",children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Generation"}),r.jsx("div",{className:"mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100",children:o.generation??"N/A"})]}),r.jsxs("div",{className:"rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3",children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Observed · monotonic ns"}),r.jsx("div",{className:"mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100 break-all",children:o.observedAtMonotonicNs??"N/A"})]}),r.jsxs("div",{className:"rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3",children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Age"}),r.jsx("div",{className:"mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100",children:o.ageMs===null?"N/A":o.ageMs.toFixed(3)+" ms"})]}),r.jsxs("div",{className:"rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3",children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Freshness"}),r.jsxs("div",{className:"mt-1 flex items-center gap-1.5 font-semibold",children:[r.jsx(qh,{className:"w-3.5 h-3.5"}),r.jsx("span",{className:o.freshness==="fresh"?"text-emerald-700 dark:text-emerald-300":"text-amber-700 dark:text-amber-300",children:o.freshness})]})]})]}),r.jsx("div",{className:"space-y-2",children:o.bindings.map((d,S)=>r.jsxs("article",{className:"rounded-xl border border-slate-900/[0.06] dark:border-white/[0.08] p-3.5 space-y-3",children:[r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2",children:[r.jsxs("div",{className:"flex items-center gap-2 text-xs font-semibold text-slate-800 dark:text-slate-100",children:[r.jsx(Jc,{className:"w-3.5 h-3.5"}),"Semantic Binding ",S+1]}),r.jsx("span",{className:"rounded-full px-2.5 py-1 text-[10px] font-bold uppercase tracking-wide "+Fy(d.bindingState),children:d.bindingState})]}),r.jsxs("div",{className:"grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs",children:[r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Domain"}),r.jsx("div",{className:"mt-1 font-semibold text-slate-900 dark:text-slate-100",children:d.domain})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Assessment"}),r.jsx("div",{className:"mt-1 font-semibold text-slate-900 dark:text-slate-100",children:d.domainAssessment.state})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Confidence"}),r.jsxs("div",{className:"mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100",children:[(d.domainAssessment.confidence*100).toFixed(1),"%"]})]}),r.jsxs("div",{children:[r.jsx("div",{className:"text-slate-500 dark:text-slate-400",children:"Corroborating relations"}),r.jsxs("div",{className:"mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100",children:[d.domainAssessment.corroboratingRelations,"/3"]})]})]}),r.jsxs("div",{className:"flex flex-wrap items-center justify-between gap-2 border-t border-slate-900/[0.05] dark:border-white/[0.06] pt-2 text-[11px] text-slate-500 dark:text-slate-400",children:[r.jsxs("span",{children:["Binding freshness: ",r.jsx("strong",{className:d.freshness==="fresh"?"text-emerald-700 dark:text-emerald-300":"text-amber-700 dark:text-amber-300",children:d.freshness}),d.ageMs===null?"":" · "+d.ageMs.toFixed(3)+" ms"]}),r.jsxs("span",{className:"inline-flex items-center gap-1",children:[r.jsx(ja,{className:"w-3.5 h-3.5"}),"Semantic validation is not mutation authority"]})]})]},d.generation+"-"+d.observedAtMonotonicNs+"-"+S))})]}):r.jsx("div",{className:"rounded-xl bg-slate-100/80 dark:bg-white/[0.04] px-4 py-3 text-xs text-slate-600 dark:text-slate-300",children:"No official semantic telemetry is available."}):r.jsx("div",{className:"rounded-xl bg-slate-100/80 dark:bg-white/[0.04] px-4 py-3 text-xs text-slate-600 dark:text-slate-300",children:"Native bridge detected, but an official schema v1 daemon status payload has not been received. Semantic bindings remain unavailable until the daemon responds."})]}),tu=o=>typeof o=="object"&&o!==null&&!Array.isArray(o),up=o=>typeof o=="number"&&Number.isFinite(o),dp=o=>o===null||up(o),Jf=o=>o==="unresolved"||o==="cpu"||o==="gpu"||o==="vm",ns=o=>typeof o=="string"&&/^[0-9]+$/.test(o),_p=o=>o==="fresh"||o==="stale",Wy=o=>o==="unresolved"||o==="candidate"||o==="corroborated"||o==="verified",$y=o=>{if(!tu(o)||!tu(o.domainAssessment))return!1;const c=o.domainAssessment;return(o.bindingState==="candidate"||o.bindingState==="validated")&&Jf(o.domain)&&Wy(c.state)&&Jf(c.domain)&&up(c.confidence)&&c.confidence>=0&&c.confidence<=1&&Number.isInteger(c.corroboratingRelations)&&c.corroboratingRelations>=0&&c.corroboratingRelations<=3&&ns(o.generation)&&ns(o.observedAtMonotonicNs)&&dp(o.ageMs)&&(o.ageMs===null||o.ageMs>=0)&&_p(o.freshness)},eg=o=>!tu(o)||o.schemaVersion!==1||o.status!=="available"&&o.status!=="unavailable"||!_p(o.freshness)||!Array.isArray(o.bindings)||!o.bindings.every($y)?!1:o.status==="unavailable"?o.generation===null&&o.observedAtMonotonicNs===null&&o.ageMs===null&&o.freshness==="stale"&&o.bindings.length===0:ns(o.generation)&&ns(o.observedAtMonotonicNs)&&dp(o.ageMs)&&(o.ageMs===null||o.ageMs>=0)&&o.bindings.length>0,Jr='BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; [ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; [ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; [ -f "$BIN" ] || BIN="/system/bin/coreflowd"',Cn="/data/adb/coreflow/coreflow.conf",tg="/data/adb/coreflow/logs/coreflowd.log",nu=o=>{const c=o.trim(),m=c.split(`
`).map(k=>k.trim()).filter(Boolean);for(let k=m.length-1;k>=0;k--){const M=m[k];if(M.startsWith("{")&&M.endsWith("}"))try{return JSON.parse(M)}catch{}}const d=c.indexOf('{"running":'),S=d>=0?d:c.indexOf("{"),q=c.lastIndexOf("}");if(S>=0&&q>S){const k=c.slice(S,q+1).replace(/\r?\n/g,"");return JSON.parse(k)}return JSON.parse(m[m.length-1]??"{}")},yl=o=>typeof o=="object"&&o!==null&&!Array.isArray(o),Xt=o=>typeof o=="number"&&Number.isFinite(o),ng=["baselineNodesSnapshotted","thermalSeverity","aiPredictedPressure10s","aiPredictedVelocity","aiRecommendedScale","aiConfidence","aiSamples","tunerUtilityDelta","tunerTrials","tunerImproved","tunerRolledBack"],ag=["cpuPressurePct","memoryPressurePct","ioPressurePct","socCompositeCelsius","cpuAverageCelsius","cpuPeakCelsius","gpuTempCelsius","batteryTempCelsius","skinTempCelsius","thermalHeadroom"],ig=["cpuPressureValid","memoryPressureValid","ioPressureValid","thermalPolicyAvailable","thermalGuardActive","thermalDataDegraded","thermalFallbackActive","thermalHeadroomValid","aiOnnxActive"],lg=["thermalSource","thermalSensor","aiBackend","tunerRuntimeStage","tunerStage","tunerIntent","tunerCandidateId","tunerCandidateLabel","tunerRationale","tunerVerdict"],Wr=(o,c)=>c.every(m=>typeof o[m]=="string"),Fc=(o,c)=>c.every(m=>Xt(o[m])),Wf=(o,c)=>c.every(m=>typeof o[m]=="boolean"),rg={thermalSensors:o=>yl(o)&&Wr(o,["id","type"])&&Xt(o.category)&&(o.celsius===null||Xt(o.celsius))&&Wf(o,["valid","policyAvailable"])&&(o.velocityCelsiusPerSec===void 0||o.velocityCelsiusPerSec===null||Xt(o.velocityCelsiusPerSec)),blockQueues:o=>yl(o)&&Wr(o,["device","activeScheduler","baselineScheduler"])&&Array.isArray(o.availableSchedulers)&&o.availableSchedulers.every(c=>typeof c=="string")&&Fc(o,["readAheadKb","nrRequests","rqAffinity","readExpireMs","writesStarved","fifoBatch"]),tunerCandidates:o=>yl(o)&&Wr(o,["id","label","domain"])&&Fc(o,["trials","successes","rollbacks","meanDelta","ucbScore","cooldownTicks"]),tunerEvaluatedCandidates:o=>yl(o)&&Wr(o,["id","domain","intent","rejectionReason"])&&Fc(o,["utilityScore","empiricalScore","confidence","riskPenalty","thermalPenalty"])&&Wf(o,["eligible","selected"])},sg=o=>{const c=(m,d)=>{o[m]!==void 0&&!d&&delete o[m]};for(const m of ng)c(m,Xt(o[m]));for(const m of ag)c(m,o[m]===null||Xt(o[m]));for(const m of ig)c(m,typeof o[m]=="boolean");for(const m of lg)c(m,typeof o[m]=="string");for(const[m,d]of Object.entries(rg)){const S=o[m];c(m,Array.isArray(S)&&S.every(d))}c("vulkan",yl(o.vulkan))},og=o=>{const c=nu(o);if(typeof c!="object"||c===null)throw new Error("Invalid native status");const m=c;if(typeof m.running!="boolean"||!Xt(m.pid)||!Xt(m.uclampMin)||!Xt(m.uclampMax)||typeof m.uclampSupported!="boolean"||m.uclampPolicyMin!==void 0&&!Xt(m.uclampPolicyMin)||m.uclampPolicyMax!==void 0&&!Xt(m.uclampPolicyMax)||m.uclampTargetPath!==void 0&&typeof m.uclampTargetPath!="string"||m.uclampEncoding!==void 0&&typeof m.uclampEncoding!="string"||!Xt(m.socTempCelsius)&&m.socTempCelsius!==null||typeof m.recoveryPending!="boolean")throw new Error("Unexpected daemon status schema");if(m.semanticEvidence!==void 0&&!eg(m.semanticEvidence))throw new Error("Unexpected semanticEvidence schema; expected native schemaVersion 1");if(m.directAdaptive!==void 0&&!Sy(m.directAdaptive))throw new Error("Unexpected directAdaptive status schema");return sg(c),m},yi=async o=>{const c=await bi(o);if(c.errno!==0)throw new Error(c.stderr||c.stdout||"Native command failed");return c.stdout},cg=()=>{var me,he;const[o]=P.useState(()=>as()),[c,m]=P.useState(()=>gy()),[d,S]=P.useState(null),[q,k]=P.useState(null),[M,O]=P.useState(null),[T,C]=P.useState(!1),[y,z]=P.useState(!1),Y=o&&y,[L,I]=P.useState(()=>{const U=Ey();return o?{...U,engineActive:!1,uclampMin:0,uclampMax:0,uclampSupported:!1,socTempCelsius:-1,cpuPressurePct:0,memoryPressurePct:0,ioPressurePct:0,cpuPressureValid:!1,memoryPressureValid:!1,ioPressureValid:!1,baselineNodesSnapshotted:0,baselineStatus:"restored",transactionStage:"PLANNED",modeLabel:"Awaiting native status",modeDescription:"Waiting for coreflowd runtime status",vulkan:gl($c)}:U}),[J,K]=P.useState(ts.thermalTripCelsius),[ae,Z]=P.useState("dashboard"),[_e,Se]=P.useState(()=>({...$c})),[V,H]=P.useState(null),[ne,De]=P.useState(()=>o?[]:jy()),[Be,Me]=P.useState(!1),[we,Je]=P.useState({socModel:"Unavailable",kernelVersion:"Unavailable",onlineCpus:"Unavailable",gpuType:"Unavailable"});P.useEffect(()=>{o&&yi(`${Jr}; "$BIN" --capabilities`).then(U=>{const R=nu(U);R&&["socModel","kernelVersion","onlineCpus","gpuType"].every(N=>typeof R[N]=="string")&&Je(R)}).catch(()=>{})},[o]);const[it,Ve]=P.useState(!1),D=c.theme==="dark"||c.theme==="system"&&typeof window<"u"&&((me=window.matchMedia)==null?void 0:me.call(window,"(prefers-color-scheme: dark)").matches);P.useEffect(()=>{op(c)},[c]);const te=()=>{const R={...c,theme:D?"light":"dark"};m(R),xy(R)},Q=P.useRef(L);Q.current=L;const ke=P.useRef(0),ve=P.useCallback((U,R,N)=>{ke.current+=1;const ge={id:`evt-${Date.now()}-${ke.current}`,timestamp:new Date().toTimeString().slice(0,8),type:U,tag:R,message:N};De(_t=>[ge,..._t].slice(0,50))},[]),We=P.useCallback(async()=>{var U,R;try{const N=og(await yi(`${Jr}; "$BIN" --status`));k(N.semanticEvidence??null),z(N.semanticEvidence!==void 0);const ge=N.socCompositeCelsius??N.socTempCelsius??-1,_t=((U=N.directAdaptive)==null?void 0:U.thermalGuard)??N.thermalGuardActive??(N.thermalFallbackActive?ge>=J:(N.thermalSeverity??0)>=3),yt=(R=N.directAdaptive)==null?void 0:R.mode,Le=yt?{SCREEN_OFF:"Screen Off / Deep Idle",BALANCED_SWEET_SPOT:"Balanced Sweet-Spot",ACTIVE_LOAD:"Active Load / Gaming",THERMAL_GUARD:"Thermal Guard",DISCOVERY:"Capability Discovery"}[yt]??yt:void 0;return I(F=>{var ce,Ge,gt,$e,en;return{...F,engineActive:N.running&&(((ce=N.directAdaptive)==null?void 0:ce.active)??!0),directAdaptive:N.directAdaptive,uclampMin:N.uclampMin,uclampMax:N.uclampMax,uclampSupported:N.uclampSupported,uclampPolicyMin:N.uclampPolicyMin??F.uclampPolicyMin,uclampPolicyMax:N.uclampPolicyMax??F.uclampPolicyMax,uclampTargetPath:N.uclampTargetPath??F.uclampTargetPath,uclampEncoding:N.uclampEncoding??F.uclampEncoding,cpuPressurePct:N.cpuPressurePct??0,memoryPressurePct:N.memoryPressurePct??0,ioPressurePct:N.ioPressurePct??0,cpuPressureValid:N.cpuPressureValid??N.cpuPressurePct!=null,memoryPressureValid:N.memoryPressureValid??N.memoryPressurePct!=null,ioPressureValid:N.ioPressureValid??N.ioPressurePct!=null,baselineNodesSnapshotted:N.baselineNodesSnapshotted??F.baselineNodesSnapshotted,socTempCelsius:ge,socCompositeCelsius:N.socCompositeCelsius??ge,cpuAverageCelsius:N.cpuAverageCelsius??-1,cpuPeakCelsius:N.cpuPeakCelsius??-1,gpuTempCelsius:N.gpuTempCelsius??-1,batteryTempCelsius:N.batteryTempCelsius??-1,skinTempCelsius:N.skinTempCelsius??-1,thermalGuardActive:_t,thermalDataDegraded:N.thermalDataDegraded??!1,thermalFallbackActive:N.thermalFallbackActive??!1,thermalHeadroomValid:N.thermalHeadroomValid??!1,thermalHeadroom:N.thermalHeadroom??null,thermalSource:N.thermalSource??F.thermalSource,thermalSeverity:N.thermalSeverity??F.thermalSeverity,thermalSensors:N.thermalSensors??F.thermalSensors,aiBackend:N.aiBackend??F.aiBackend,aiOnnxActive:N.aiOnnxActive??F.aiOnnxActive,aiPredictedPressure10s:N.aiPredictedPressure10s??F.aiPredictedPressure10s,aiPredictedVelocity:N.aiPredictedVelocity??F.aiPredictedVelocity,aiRecommendedScale:N.aiRecommendedScale??F.aiRecommendedScale,aiConfidence:N.aiConfidence??F.aiConfidence,aiSamples:N.aiSamples??F.aiSamples,tunerRuntimeStage:N.tunerRuntimeStage??F.tunerRuntimeStage,tunerStage:N.tunerStage??F.tunerStage,tunerIntent:N.tunerIntent??F.tunerIntent,tunerCandidateId:N.tunerCandidateId??F.tunerCandidateId,tunerCandidateLabel:N.tunerCandidateLabel??F.tunerCandidateLabel,tunerRationale:N.tunerRationale??F.tunerRationale,tunerVerdict:N.tunerVerdict??F.tunerVerdict,tunerUtilityDelta:N.tunerUtilityDelta??F.tunerUtilityDelta,tunerTrials:N.tunerTrials??F.tunerTrials,tunerImproved:N.tunerImproved??F.tunerImproved,tunerRolledBack:N.tunerRolledBack??F.tunerRolledBack,blockQueues:N.blockQueues??F.blockQueues,tunerCandidates:N.tunerCandidates??F.tunerCandidates,tunerEvaluatedCandidates:N.tunerEvaluatedCandidates??F.tunerEvaluatedCandidates,vulkan:N.vulkan??F.vulkan,baselineStatus:N.recoveryPending||((Ge=N.directAdaptive)==null?void 0:Ge.active)===!0&&N.directAdaptive.baselineNodes>0?"protected":"restored",transactionStage:N.running?N.recoveryPending?"RETAINED":"VERIFIED":"PLANNED",modeLabel:N.running?Le??(_t?"Thermal Guard":"Direct Adaptive runtime active"):"Daemon Stopped",modeDescription:N.running?(gt=N.directAdaptive)!=null&&gt.journalFault?"Journal fault latched; all further Direct Adaptive mutations are halted until baseline recovery.":($e=N.directAdaptive)!=null&&$e.ownershipLosses?"OEM contention isolated at affected nodes; independent capabilities continue.":(en=N.directAdaptive)!=null&&en.active?"Direct Baseline with capability-derived Sweet-Spot policy and verified readback.":"Daemon is running; Direct Adaptive discovery or baseline preparation is unavailable.":"OEM baseline restored",timestamp:Date.now()}}),S(null),N}catch(N){throw z(!1),k(null),I(ge=>({...ge,directAdaptive:void 0})),N}},[J]);P.useEffect(()=>{if(!o)return;let U=!1;const R=async()=>{const Le=await bi(`cat ${Cn} 2>/dev/null`);if(U||Le.errno!==0)return;const F=ky(Le.stdout,"thermal_trip_celsius");F!==null&&K(Math.max(30,Math.min(80,F)));const ce=eu(Le.stdout,"vulkan_layer_mode"),Ge=ce==="safe_hardware"||ce==="full_capability_override"?"safe_hardware":"disabled",gt=$e=>{const en=eu(Le.stdout,$e);return en==="true"?!0:en==="false"?!1:null};Se($e=>({...$e,mode:Ge,enableSkiavkThreaded:gt("enable_skiavk_threaded")??$e.enableSkiavkThreaded,enableAdaptiveVrs:gt("enable_adaptive_vrs")??$e.enableAdaptiveVrs,enablePipelineCachePrewarm:gt("enable_pipeline_cache_prewarm")??$e.enablePipelineCachePrewarm}))};let N=!1,ge="";const _t=async()=>{if(!(N||U)){N=!0;try{await We();const Le=await bi(`/system/bin/tail -n 12 ${tg} 2>/dev/null`);if(!U&&Le.errno===0){const F=Le.stdout.trim();if(F&&F!==ge){ge=F;const ce=F.split(/\r?\n/).map(Ge=>Ge.trim()).filter(Boolean).slice(-12).reverse();De(ce.map((Ge,gt)=>({id:`native-${gt}-${Ge}`,timestamp:"--:--:--",type:Ge.includes("[Stability]")&&Ge.includes("thermal")?"THERMAL":"INFO",tag:"Daemon",message:Ge.slice(0,384)})))}}}catch(Le){U||S(String(Le))}finally{N=!1}}};R(),_t();const yt=window.setInterval(()=>void _t(),1e4);return()=>{U=!0,window.clearInterval(yt)}},[o,We]);const Ut=P.useCallback(U=>{const{next:R,event:N}=Ty(Q.current,ts,U);Q.current=R,I(R),N&&De(ge=>[N,...ge].slice(0,50))},[]);P.useEffect(()=>{if(o)return;const U=window.setInterval(()=>{Ut()},2500);return()=>window.clearInterval(U)},[o,Ut]);const mn=async U=>{const R=await bi(`cat "${Cn}" 2>/dev/null || cat /data/adb/modules/cfae_framework/system/etc/coreflow/default.conf 2>/dev/null || cat /system/etc/coreflow/default.conf 2>/dev/null || true`),N=Ny(R.stdout??"",{vulkan_layer_mode:U}),ge=N.endsWith(`
`)?N:N+`
`;let _t="COREFLOW_CONFIG";for(let F=1;ge.split(`
`).some(ce=>ce.trim()===_t);F++)_t=`COREFLOW_CONFIG_${F}`;const yt='umask 077 && [ ! -L "'+Cn+'" ] && rm -f "'+Cn+`.tmp" && cat <<'${_t}' > `+Cn+`.tmp
`+ge+_t+`
chown 0:0 `+Cn+".tmp 2>/dev/null; chmod 600 "+Cn+".tmp && mv -f "+Cn+".tmp "+Cn+" && echo COREFLOW_VULKAN_WRITTEN";if(!(await yi(yt)).includes("COREFLOW_VULKAN_WRITTEN"))throw new Error("Vulkan layer mode write not acknowledged")},p=async U=>{Ve(!0),H(null);try{const R=U.mode==="disabled"?"disabled":"safe_hardware",N={...U,mode:R};if(!o){Se(N),I(ge=>({...ge,vulkan:gl(N,ge.vulkan)})),H(R==="safe_hardware"?"Truthful Hardware Pass-Through active in preview":"OEM Passthrough (Disabled) active in preview"),ve("BOOST","VulkanLayer",`Layer mode=${R}; rendering optimizers remain unimplemented.`);return}await mn(R),Se(N),I(ge=>({...ge,vulkan:gl(N,ge.vulkan)})),await yi("killall -HUP coreflowd 2>/dev/null || true"),H("Mode saved · daemon reload requested"),We().catch(()=>{}),ve("BOOST","VulkanLayer",`Vulkan layer mode changed to ${R}; no system tuning policy was changed.`)}catch(R){H("Save failed"),S(String(R))}finally{Ve(!1)}},A=P.useCallback(async()=>{try{const U=await yi(`${Jr}; "$BIN" --journal`),R=nu(U);O(JSON.stringify(R,null,2)),S(null)}catch(U){S("Unable to read the mutation journal: "+String(U))}},[]),W=P.useCallback(async()=>{if(window.confirm("Restore every CoreFlow-owned node to its captured OEM baseline and stop coreflowd?")){C(!0);try{await yi(`${Jr}; "$BIN" --rollback-baseline`),O(null),await We(),ve("BASELINE","Baseline","OEM baseline rollback requested from the Direct Adaptive audit panel.")}catch(U){S("OEM baseline restore failed: "+String(U))}finally{C(!1)}}},[ve,We]),$=[{id:"dashboard",label:"Overview",Icon:xl},{id:"thermal",label:"Thermal",Icon:su},{id:"hardware",label:"Hardware",Icon:es},{id:"console",label:"Audit & Diagnostics",Icon:cu},{id:"validation",label:"Validation",Icon:Oh}];return r.jsxs("div",{className:"min-h-screen relative overflow-x-hidden bg-[#f7f9fc] dark:bg-[#0e1117] text-slate-900 dark:text-slate-100 flex flex-col font-sans transition-colors duration-200 antialiased",children:[r.jsx("div",{"aria-hidden":"true",className:"pointer-events-none fixed top-12 left-1/4 w-[28rem] h-[28rem] rounded-full bg-[#d3e3fd]/45 dark:bg-[#0842a0]/15 blur-3xl -z-10"}),r.jsx("div",{"aria-hidden":"true",className:"pointer-events-none fixed bottom-12 right-1/4 w-[26rem] h-[26rem] rounded-full bg-slate-200/55 dark:bg-slate-800/20 blur-3xl -z-10"}),r.jsx("div",{role:"status",className:"px-4 py-2 text-center text-xs backdrop-blur-md border-b border-slate-900/[0.04] dark:border-white/[0.04] "+(o&&Y?"bg-emerald-100 text-emerald-900 dark:bg-emerald-950/50 dark:text-emerald-200":"bg-amber-100 text-amber-950 dark:bg-amber-950/50 dark:text-amber-200"),children:o?Y?"Connected · Native bridge and official daemon status with semanticEvidence schema v1 received.":"Native bridge detected · Awaiting an official daemon status payload; semantic telemetry is unavailable.":"Simulation Mode · Browser-only preview; synthetic values are labelled (sim) and are not kernel audit evidence."}),d&&r.jsx("div",{role:"alert",className:"px-4 py-2 bg-rose-100 text-rose-900 dark:bg-rose-950/80 dark:text-rose-200 text-xs font-medium text-center",children:d}),r.jsx(zy,{isDark:D,onToggleTheme:te,modeLabel:L.modeLabel,nativeMode:o,nativeConnected:Y,onOpenArchitectureStudio:()=>Me(!0)}),r.jsxs("main",{className:"pixel-main flex-1 w-full mx-auto px-4 sm:px-6 py-6 sm:py-8 space-y-6",children:[r.jsx("nav",{"aria-label":"Primary sections",className:"pixel-nav flex items-center gap-1.5 overflow-x-auto no-scrollbar max-w-full",children:$.map(({id:U,label:R,Icon:N})=>{const ge=ae===U;return r.jsxs("button",{type:"button","aria-current":ge?"page":void 0,onClick:()=>Z(U),className:"pixel-nav-item flex items-center gap-2 whitespace-nowrap select-none "+(ge?"is-active":""),children:[r.jsx(N,{className:"w-4 h-4 shrink-0"}),r.jsx("span",{children:R})]},U)})}),ae==="dashboard"&&r.jsxs("div",{className:"pixel-page space-y-6 m3-stage",children:[r.jsxs("section",{className:"space-y-3","aria-label":"Autonomous runtime state",children:[r.jsxs("div",{className:"flex flex-col gap-2 sm:flex-row sm:items-end sm:justify-between",children:[r.jsxs("div",{children:[r.jsx("p",{className:"text-[11px] font-semibold uppercase tracking-[0.18em] text-[#0b57d0] dark:text-[#a8c7fa]",children:"CFAE · Autonomous system optimization"}),r.jsx("h1",{className:"mt-1 text-2xl font-semibold tracking-tight text-slate-900 dark:text-slate-100 sm:text-3xl",children:"Live operating state"}),r.jsx("p",{className:"mt-1 max-w-2xl text-sm leading-relaxed text-slate-600 dark:text-slate-400",children:"Capability discovery, evidence-led tuning, and verified actuator ownership. The dashboard reflects native daemon state; selection remains autonomous and safety-gated."})]}),r.jsxs("div",{className:"inline-flex w-fit items-center gap-2 rounded-full border border-slate-900/[0.07] bg-white/70 px-3 py-2 text-[11px] font-semibold text-slate-600 dark:border-white/[0.08] dark:bg-white/[0.04] dark:text-slate-300",children:[r.jsx("span",{className:"h-2 w-2 rounded-full "+(L.engineActive?"bg-emerald-500":"bg-slate-400")}),L.engineActive?"Daemon operating autonomously":"Daemon standby"]})]}),!o&&r.jsx(Oy,{telemetry:L,nativeMode:!1,thermalTripCelsius:J}),o&&r.jsx(Ry,{status:L.directAdaptive,nativeMode:!0,journalOutput:M,restoringBaseline:T,onViewJournal:A,onRestoreBaseline:W})]}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"pixel-metric m3-card flex items-start gap-3 p-4",children:[r.jsx("div",{className:"rounded-xl bg-[#d3e3fd] p-2.5 text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:r.jsx(Wc,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Thermal headroom"}),r.jsx("div",{className:"mt-1 text-lg font-semibold tabular-nums text-slate-900 dark:text-slate-100",children:L.thermalHeadroomValid&&L.thermalHeadroom!=null?`${Math.round(L.thermalHeadroom*100)}%`:"Unavailable"}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:L.thermalGuardActive?"Thermal guard engaged":"OEM thermal policy respected"})]})]}),r.jsxs("div",{className:"pixel-metric m3-card flex items-start gap-3 p-4",children:[r.jsx("div",{className:"rounded-xl bg-[#d3e3fd] p-2.5 text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:r.jsx($r,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Resource pressure"}),r.jsx("div",{className:"mt-1 text-lg font-semibold tabular-nums text-slate-900 dark:text-slate-100",children:L.cpuPressureValid||L.memoryPressureValid||L.ioPressureValid?`${Math.max(L.cpuPressureValid?L.cpuPressurePct:0,L.memoryPressureValid?L.memoryPressurePct:0,L.ioPressureValid?L.ioPressurePct:0).toFixed(1)}%`:"Unavailable"}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Highest available PSI avg10"})]})]}),r.jsxs("div",{className:"pixel-metric m3-card flex items-start gap-3 p-4",children:[r.jsx("div",{className:"rounded-xl bg-[#d3e3fd] p-2.5 text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]",children:r.jsx(ou,{className:"h-4 w-4"})}),r.jsxs("div",{className:"min-w-0",children:[r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Mutation ownership"}),r.jsx("div",{className:"mt-1 text-lg font-semibold text-slate-900 dark:text-slate-100",children:L.baselineStatus==="protected"?"Journal armed":"Guarded"}),r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Baseline + readback protection"})]})]})]}),r.jsx(Jy,{evidence:q,nativeConnected:Y,simulationMode:!o}),!o&&r.jsx(Pf,{nodesSnapshotted:L.baselineNodesSnapshotted,nativeMode:!1,baselineStatus:L.baselineStatus}),r.jsx(Fr,{logs:ne,nativeMode:o})]}),ae==="thermal"&&r.jsxs("div",{className:"pixel-page space-y-6 m3-stage",children:[r.jsxs("header",{className:"pixel-page-heading",children:[r.jsx("p",{className:"pixel-page-eyebrow",children:"Protection domain"}),r.jsx("h1",{className:"pixel-page-title",children:"Thermal intelligence"}),r.jsx("p",{className:"pixel-page-description",children:"Sensor evidence and OEM thermal constraints are displayed for transparency. The daemon selects mitigation automatically."})]}),r.jsx(Iy,{telemetry:L,nativeMode:o,thermalTripCelsius:J}),r.jsx(Fr,{logs:ne.filter(U=>U.type==="THERMAL"||U.tag.toLowerCase().includes("thermal")),nativeMode:o})]}),ae==="hardware"&&r.jsxs("div",{className:"pixel-page space-y-6 m3-stage",children:[r.jsxs("header",{className:"pixel-page-heading",children:[r.jsx("p",{className:"pixel-page-eyebrow",children:"Capability discovery"}),r.jsx("h1",{className:"pixel-page-title",children:"Hardware & resource profile"}),r.jsx("p",{className:"pixel-page-description",children:"Detected hardware is evidence, not automatic mutation authority. CoreFlow adapts only through validated operational contracts."})]}),r.jsxs("section",{className:"m3-card space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(wi,{className:"h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("h2",{className:"font-semibold",children:"Device identity"})]}),r.jsx("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[["SoC",we.socModel],["GPU",we.gpuType],["Kernel",we.kernelVersion],["Online CPUs",we.onlineCpus]].map(([U,R])=>r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:U}),r.jsx("div",{className:"mt-1 break-words font-mono text-sm font-semibold text-slate-900 dark:text-slate-100",children:R})]},U))})]}),r.jsxs("section",{className:"m3-card space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{children:[r.jsx("p",{className:"text-[11px] font-semibold uppercase tracking-[0.16em] text-[#0b57d0] dark:text-[#a8c7fa]",children:"Kernel contract model"}),r.jsx("h2",{className:"mt-1 text-lg font-semibold",children:"Discovery does not grant mutation authority"}),r.jsx("p",{className:"mt-1 text-xs leading-relaxed text-slate-600 dark:text-slate-400",children:"Every discovered attribute receives an explicit contract classification. Unknown nodes stay visible for hardware understanding but fail closed for automated writes."})]}),r.jsx("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-2",children:[{name:"RUNTIME_TUNABLE",tone:"bg-emerald-100 text-emerald-800 dark:bg-emerald-950/50 dark:text-emerald-300",detail:"Only class eligible for tuning after ABI, range, access and baseline verification."},{name:"OBSERVATION_ONLY",tone:"bg-sky-100 text-sky-800 dark:bg-sky-950/50 dark:text-sky-300",detail:"Hardware limits, topology, alignment and telemetry contribute to the fingerprint."},{name:"TEST_ONLY",tone:"bg-amber-100 text-amber-800 dark:bg-amber-950/50 dark:text-amber-300",detail:"Writable testing interface, excluded from daily performance optimization."},{name:"PROTECTED",tone:"bg-rose-100 text-rose-800 dark:bg-rose-950/50 dark:text-rose-300",detail:"Integrity-critical or system-sensitive attribute; automatic mutation prohibited."}].map(U=>r.jsxs("div",{className:"rounded-2xl border border-slate-900/[0.06] p-4 dark:border-white/[0.07]",children:[r.jsx("span",{className:"inline-flex rounded-full px-2.5 py-1 font-mono text-[10px] font-bold "+U.tone,children:U.name}),r.jsx("p",{className:"mt-2 text-xs leading-relaxed text-slate-600 dark:text-slate-400",children:U.detail})]},U.name))})]}),r.jsxs("section",{className:"m3-card space-y-4 p-5 sm:p-7",children:[r.jsxs("div",{className:"flex items-center gap-2",children:[r.jsx(es,{className:"h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"}),r.jsx("h2",{className:"font-semibold",children:"Autonomous hardware preservation"})]}),r.jsx("p",{className:"text-sm leading-relaxed text-slate-600 dark:text-slate-400",children:"Battery, storage, and I/O protection are evaluated from live workload and device evidence. No manual thresholds or scheduler presets are exposed here; unsupported nodes remain observation-only until they earn mutation authority."}),r.jsxs("div",{className:"grid grid-cols-1 gap-3 sm:grid-cols-3",children:[r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Battery temperature"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",children:L.batteryTempCelsius!=null&&L.batteryTempCelsius>=0?`${L.batteryTempCelsius.toFixed(1)} °C`:"N/A"})]}),r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Block queues discovered"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",children:((he=L.blockQueues)==null?void 0:he.length)??0})]}),r.jsxs("div",{className:"m3-tonal-block p-4",children:[r.jsx("div",{className:"text-[11px] text-slate-500 dark:text-slate-400",children:"Baseline snapshots"}),r.jsx("div",{className:"mt-1 font-mono text-lg font-semibold",children:L.baselineNodesSnapshotted})]})]})]}),r.jsx(Pf,{nodesSnapshotted:L.baselineNodesSnapshotted,nativeMode:o,baselineStatus:L.baselineStatus})]}),ae==="vulkan"&&r.jsxs("div",{className:"pixel-page space-y-6 m3-stage",children:[r.jsxs("header",{className:"pixel-page-heading",children:[r.jsx("p",{className:"pixel-page-eyebrow",children:"User opt-in subsystem"}),r.jsx("h1",{className:"pixel-page-title",children:"Vulkan diagnostics & layer"}),r.jsx("p",{className:"pixel-page-description",children:"Vulkan layer activation is the only manual feature switch. All system optimization domains remain autonomous."})]}),r.jsx(Py,{vulkanTelemetry:L.vulkan,vulkanConfig:_e,nativeMode:o,onSaveVulkanConfig:p,isSaving:it,saveMessage:V}),r.jsx(Fr,{logs:ne.filter(U=>U.tag.toLowerCase().includes("vulkan")),nativeMode:o})]}),ae==="console"&&r.jsxs("div",{className:"pixel-page space-y-6 m3-stage",children:[r.jsxs("header",{className:"pixel-page-heading",children:[r.jsx("p",{className:"pixel-page-eyebrow",children:"Runtime transparency"}),r.jsx("h1",{className:"pixel-page-title",children:"Audit & diagnostics"}),r.jsx("p",{className:"pixel-page-description",children:"Read-only runtime evidence, decisions, and diagnostic output."})]}),r.jsx(Qy,{nativeMode:o,onRefreshTelemetry:async()=>{await We()}}),r.jsx(Fr,{logs:ne,nativeMode:o})]}),ae==="validation"&&r.jsxs("div",{className:"pixel-page space-y-6 m3-stage",children:[r.jsxs("header",{className:"pixel-page-heading",children:[r.jsx("p",{className:"pixel-page-eyebrow",children:"Evidence, not assumptions"}),r.jsx("h1",{className:"pixel-page-title",children:"Device validation"}),r.jsx("p",{className:"pixel-page-description",children:"Inspect the live validation contract and native subsystem readiness."})]}),r.jsx(Zy,{nativeMode:o})]})]}),r.jsx(Yy,{isOpen:Be,onClose:()=>Me(!1),...we}),r.jsx("footer",{className:"pixel-footer text-xs text-slate-500 dark:text-slate-400",children:r.jsxs("div",{className:"max-w-6xl mx-auto px-4 sm:px-6 flex flex-col sm:flex-row items-center justify-between gap-2",children:[r.jsxs("div",{children:[r.jsx("span",{className:"font-semibold text-slate-700 dark:text-slate-300",children:"CFAE System Framework v4.0.0"})," · ","Autonomous Mutation Safety & Coordinated Optimization"]}),r.jsx("div",{className:"text-[11px] font-mono",children:"ISO C++23 · Rust 2024 · Kotlin 2.1 · Readback Verified Recovery"})]})})]})};class ug extends P.Component{constructor(){super(...arguments);Vc(this,"state",{hasError:!1,error:null});Vc(this,"handleReset",()=>{this.setState({hasError:!1,error:null}),window.location.reload()})}static getDerivedStateFromError(m){return{hasError:!0,error:m}}componentDidCatch(m,d){console.error("[CoreFlow WebUI] Uncaught component error:",m,d)}render(){return this.state.hasError?r.jsx("div",{className:"min-h-screen bg-slate-50 dark:bg-[#090d16] text-slate-900 dark:text-slate-100 flex items-center justify-center p-4",children:r.jsxs("div",{className:"max-w-md w-full rounded-3xl bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 p-6 shadow-xl text-center",children:[r.jsx("div",{className:"w-12 h-12 rounded-2xl bg-rose-500/10 border border-rose-500/20 text-rose-500 flex items-center justify-center mx-auto mb-4",children:r.jsx(vy,{className:"w-6 h-6"})}),r.jsx("h2",{className:"text-xl font-bold tracking-tight text-slate-900 dark:text-white mb-2",children:"Dashboard Encountered an Issue"}),r.jsx("p",{className:"text-xs text-slate-500 dark:text-slate-400 mb-4 leading-relaxed",children:"CoreFlow caught a runtime error. Your device daemon settings remain intact and safe."}),this.state.error&&r.jsx("pre",{className:"p-3 mb-4 rounded-xl bg-slate-100 dark:bg-slate-950 text-[11px] font-mono text-slate-700 dark:text-slate-300 text-left overflow-auto max-h-32",children:this.state.error.message}),r.jsxs("button",{onClick:this.handleReset,className:"inline-flex items-center gap-2 px-5 py-2.5 rounded-full bg-sky-500 hover:bg-sky-400 text-slate-950 font-semibold text-xs transition-transform active:scale-95 shadow-md shadow-sky-500/20",children:[r.jsx(ap,{className:"w-3.5 h-3.5"}),r.jsx("span",{children:"Reload Dashboard"})]})]})}):this.props.children}}xh.createRoot(document.getElementById("root")).render(r.jsx(P.StrictMode,{children:r.jsx(ug,{children:r.jsx(cg,{})})}));
