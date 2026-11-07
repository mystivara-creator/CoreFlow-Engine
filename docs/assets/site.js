(() => {
  "use strict";

  const toggle = document.querySelector(".menu-toggle");
  const nav = document.querySelector(".site-nav");
  const header = document.querySelector(".topbar");

  if (toggle && nav) {
    const closeMenu = () => {
      toggle.setAttribute("aria-expanded", "false");
      toggle.setAttribute("aria-label", "Buka navigasi");
      nav.classList.remove("open");
    };
    toggle.addEventListener("click", () => {
      const open = toggle.getAttribute("aria-expanded") === "true";
      toggle.setAttribute("aria-expanded", String(!open));
      toggle.setAttribute("aria-label", open ? "Buka navigasi" : "Tutup navigasi");
      nav.classList.toggle("open", !open);
    });
    nav.querySelectorAll("a").forEach((link) => link.addEventListener("click", closeMenu));
    document.addEventListener("keydown", (event) => {
      if (event.key === "Escape") closeMenu();
    });
  }

  const year = document.getElementById("year");
  if (year) year.textContent = String(new Date().getFullYear());

  // Interactive four-stage pipeline in the hero.
  const flowSteps = [...document.querySelectorAll(".flow-step")];
  const stepTitle = document.getElementById("art-step-title");
  const stepDescription = document.getElementById("art-step-description");
  const pipelineCopy = [
    ["01 / CAPABILITY DISCOVERY", "CoreFlow mengenali kemampuan perangkat dan semantik antarmuka sistem sebelum menentukan tindakan."],
    ["02 / WORKLOAD EVALUATION", "CPU, memori, I/O, workload, dan kondisi termal dievaluasi untuk memilih tujuan optimasi yang sesuai."],
    ["03 / CONTROLLED ACTUATION", "Perubahan dikoordinasikan lintas domain, menggunakan baseline dan pemeriksaan kepemilikan sebelum penulisan."],
    ["04 / OUTCOME VERIFICATION", "Readback, settling, dan hasil dievaluasi. Perubahan hanya dipertahankan jika terbukti aman dan bermanfaat."]
  ];
  flowSteps.forEach((button, index) => {
    button.addEventListener("click", () => {
      flowSteps.forEach((item, itemIndex) => {
        const active = itemIndex === index;
        item.classList.toggle("is-active", active);
        item.setAttribute("aria-pressed", String(active));
      });
      if (stepTitle) stepTitle.textContent = pipelineCopy[index][0];
      if (stepDescription) stepDescription.textContent = pipelineCopy[index][1];
    });
  });

  const progress = document.getElementById("scroll-progress-bar");
  const backTop = document.querySelector(".back-to-top");
  let scrollFrame = 0;
  const updateScrollUI = () => {
    scrollFrame = 0;
    const maxScroll = document.documentElement.scrollHeight - window.innerHeight;
    const ratio = maxScroll > 0 ? Math.min(1, Math.max(0, window.scrollY / maxScroll)) : 0;
    if (progress) progress.style.transform = `scaleX(${ratio})`;
    if (header) header.classList.toggle("is-scrolled", window.scrollY > 18);
    if (backTop) {
      const visible = window.scrollY > 520;
      backTop.hidden = !visible && !backTop.classList.contains("is-visible");
      if (visible) backTop.hidden = false;
      requestAnimationFrame(() => backTop.classList.toggle("is-visible", visible));
    }
  };
  const onScroll = () => {
    if (!scrollFrame) scrollFrame = requestAnimationFrame(updateScrollUI);
  };
  window.addEventListener("scroll", onScroll, { passive: true });
  window.addEventListener("resize", onScroll, { passive: true });
  updateScrollUI();

  if (backTop) backTop.addEventListener("click", () => window.scrollTo({ top: 0, behavior: "smooth" }));

  // Scroll-aware navigation: mark the section currently occupying the viewport.
  const sectionLinks = [...document.querySelectorAll('.site-nav a[href^="#"]')];
  const sectionPairs = sectionLinks
    .map((link) => ({ link, section: document.querySelector(link.getAttribute("href")) }))
    .filter((item) => item.section);
  if ("IntersectionObserver" in window && sectionPairs.length) {
    const sectionObserver = new IntersectionObserver((entries) => {
      const visible = entries.filter((entry) => entry.isIntersecting)
        .sort((a, b) => b.intersectionRatio - a.intersectionRatio)[0];
      if (!visible) return;
      sectionPairs.forEach(({ link, section }) => {
        const active = section === visible.target;
        if (active) link.setAttribute("aria-current", "location");
        else link.removeAttribute("aria-current");
        link.classList.toggle("is-active", active);
      });
    }, { rootMargin: "-22% 0px -58% 0px", threshold: [0, .15, .35, .6] });
    sectionPairs.forEach(({ section }) => sectionObserver.observe(section));
  }

  const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)");
  if (!reduceMotion.matches && "IntersectionObserver" in window) {
    const revealTargets = document.querySelectorAll(
      ".intro-content, .feature-card, .workflow-copy, .workflow-step, .compatibility-grid > div, .download-panel > div, .faq-grid > div"
    );
    if (revealTargets.length) {
      document.documentElement.classList.add("motion-ready");
      const observer = new IntersectionObserver((entries, activeObserver) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            entry.target.classList.add("is-visible");
            activeObserver.unobserve(entry.target);
          }
        });
      }, { threshold: .12, rootMargin: "0px 0px -36px 0px" });
      revealTargets.forEach((element) => {
        element.classList.add("reveal");
        observer.observe(element);
      });
    }
  }

  // Subtle card tilt + magnetic CTAs only on fine pointers.
  const finePointer = window.matchMedia("(hover: hover) and (pointer: fine)").matches;
  if (finePointer && !reduceMotion.matches) {
    document.querySelectorAll(".feature-card").forEach((card) => {
      card.addEventListener("pointermove", (event) => {
        const rect = card.getBoundingClientRect();
        const x = (event.clientX - rect.left) / rect.width;
        const y = (event.clientY - rect.top) / rect.height;
        card.style.setProperty("--pointer-x", `${(x * 100).toFixed(1)}%`);
        card.style.setProperty("--pointer-y", `${(y * 100).toFixed(1)}%`);
        card.style.setProperty("--tilt-x", `${((x - .5) * 3.2).toFixed(2)}deg`);
        card.style.setProperty("--tilt-y", `${((.5 - y) * 3.2).toFixed(2)}deg`);
      });
      card.addEventListener("pointerleave", () => {
        card.style.setProperty("--tilt-x", "0deg");
        card.style.setProperty("--tilt-y", "0deg");
      });
    });

    document.querySelectorAll(".hero-actions .button, .download-actions .button").forEach((button) => {
      button.addEventListener("pointermove", (event) => {
        const rect = button.getBoundingClientRect();
        const x = (event.clientX - rect.left - rect.width / 2) / rect.width;
        const y = (event.clientY - rect.top - rect.height / 2) / rect.height;
        button.style.setProperty("--mag-x", `${(x * 5).toFixed(1)}px`);
        button.style.setProperty("--mag-y", `${(y * 4).toFixed(1)}px`);
        button.style.transform = `translate3d(${(x * 5).toFixed(1)}px, ${(y * 4).toFixed(1)}px, 0) translateY(-2px)`;
      });
      button.addEventListener("pointerleave", () => { button.style.transform = ""; });
    });
  }
})();
