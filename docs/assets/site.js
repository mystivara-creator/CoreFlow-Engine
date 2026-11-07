(() => {
  "use strict";

  const toggle = document.querySelector(".menu-toggle");
  const nav = document.querySelector(".site-nav");

  if (toggle && nav) {
    const closeMenu = () => {
      toggle.setAttribute("aria-expanded", "false");
      toggle.setAttribute("aria-label", "Buka navigasi");
      nav.classList.remove("open");
    };

    toggle.addEventListener("click", () => {
      const isOpen = toggle.getAttribute("aria-expanded") === "true";
      toggle.setAttribute("aria-expanded", String(!isOpen));
      toggle.setAttribute("aria-label", isOpen ? "Buka navigasi" : "Tutup navigasi");
      nav.classList.toggle("open", !isOpen);
    });

    nav.querySelectorAll("a").forEach((link) => link.addEventListener("click", closeMenu));
    document.addEventListener("keydown", (event) => {
      if (event.key === "Escape") closeMenu();
    });
  }

  const year = document.getElementById("year");
  if (year) year.textContent = String(new Date().getFullYear());

  // Progressive enhancement: content stays visible if motion APIs are unavailable.
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
      }, { threshold: 0.12, rootMargin: "0px 0px -36px 0px" });

      revealTargets.forEach((element) => {
        element.classList.add("reveal");
        observer.observe(element);
      });
    }
  }
})();
