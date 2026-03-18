/* ============================================================
   Integrall Landing Page — JavaScript
   ============================================================ */

document.addEventListener('DOMContentLoaded', () => {

  // ----- Dark Mode -----
  const themeToggle = document.getElementById('theme-toggle');
  const prefersDark  = window.matchMedia('(prefers-color-scheme: dark)');

  function setTheme(dark) {
    document.documentElement.setAttribute('data-theme', dark ? 'dark' : 'light');
    themeToggle.textContent = dark ? '☀️' : '🌙';
    localStorage.setItem('theme', dark ? 'dark' : 'light');
  }

  const stored = localStorage.getItem('theme');
  if (stored) {
    setTheme(stored === 'dark');
  } else {
    setTheme(prefersDark.matches);
  }

  themeToggle.addEventListener('click', () => {
    const current = document.documentElement.getAttribute('data-theme');
    setTheme(current !== 'dark');
  });

  // ----- Sticky Nav -----
  const nav = document.querySelector('.nav');
  function checkScroll() {
    nav.classList.toggle('scrolled', window.scrollY > 40);
  }
  window.addEventListener('scroll', checkScroll, { passive: true });
  checkScroll();

  // ----- Mobile Menu -----
  const mobileToggle = document.querySelector('.mobile-toggle');
  const mobileNav    = document.querySelector('.mobile-nav');
  const mobileClose  = document.querySelector('.mobile-close');

  if (mobileToggle && mobileNav) {
    mobileToggle.addEventListener('click', () => mobileNav.classList.add('open'));
    mobileClose.addEventListener('click',  () => mobileNav.classList.remove('open'));
    mobileNav.querySelectorAll('a').forEach(link => {
      link.addEventListener('click', () => mobileNav.classList.remove('open'));
    });
  }

  // ----- Install Tabs -----
  const tabs   = document.querySelectorAll('.install-tab');
  const panels = document.querySelectorAll('.install-panel');

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      const target = tab.dataset.tab;
      tabs.forEach(t   => t.classList.toggle('active', t === tab));
      panels.forEach(p => p.classList.toggle('active', p.id === target));
    });
  });

  // ----- Copy Buttons -----
  document.querySelectorAll('.copy-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const code = btn.closest('.install-code').querySelector('pre').textContent;
      navigator.clipboard.writeText(code.trim()).then(() => {
        btn.textContent = 'Copied!';
        btn.classList.add('copied');
        setTimeout(() => {
          btn.textContent = 'Copy';
          btn.classList.remove('copied');
        }, 2000);
      });
    });
  });

  // ----- Scroll Reveal -----
  const reveals = document.querySelectorAll('.reveal');
  if ('IntersectionObserver' in window) {
    const observer = new IntersectionObserver((entries) => {
      entries.forEach(entry => {
        if (entry.isIntersecting) {
          entry.target.classList.add('visible');
          observer.unobserve(entry.target);
        }
      });
    }, { threshold: 0.12, rootMargin: '0px 0px -40px 0px' });
    reveals.forEach(el => observer.observe(el));
  } else {
    reveals.forEach(el => el.classList.add('visible'));
  }

  // ----- Smooth Scroll for Anchor Links -----
  document.querySelectorAll('a[href^="#"]').forEach(link => {
    link.addEventListener('click', e => {
      const target = document.querySelector(link.getAttribute('href'));
      if (target) {
        e.preventDefault();
        const offset = nav.offsetHeight + 16;
        const y = target.getBoundingClientRect().top + window.scrollY - offset;
        window.scrollTo({ top: y, behavior: 'smooth' });
      }
    });
  });
});
