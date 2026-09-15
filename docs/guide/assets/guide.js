/* SimFlow User Guide - navigation, on-this-page rail, and client-side search.
   No dependencies. Every feature degrades to plain HTML if this file fails. */
(function () {
  'use strict';

  var doc = document;

  /* ------------------------------------------------------------- theme */

  var THEME_KEY = 'simflow-guide-theme';

  function applyTheme(name) {
    if (name === 'light') {
      doc.documentElement.setAttribute('data-theme', 'light');
    } else {
      doc.documentElement.removeAttribute('data-theme');
    }
    var btn = doc.getElementById('themeToggle');
    if (btn) {
      var light = name === 'light';
      btn.setAttribute('aria-pressed', light ? 'true' : 'false');
      btn.setAttribute('aria-label', light ? 'Switch to dark theme' : 'Switch to light theme');
      var sun = btn.querySelector('.i-sun');
      var moon = btn.querySelector('.i-moon');
      if (sun) sun.hidden = !light;
      if (moon) moon.hidden = light;
    }
  }

  var stored = null;
  try { stored = localStorage.getItem(THEME_KEY); } catch (e) { stored = null; }
  applyTheme(stored === 'light' ? 'light' : 'dark');

  doc.addEventListener('click', function (ev) {
    var btn = ev.target.closest && ev.target.closest('#themeToggle');
    if (!btn) return;
    var next = doc.documentElement.getAttribute('data-theme') === 'light' ? 'dark' : 'light';
    applyTheme(next);
    try { localStorage.setItem(THEME_KEY, next); } catch (e) { /* private mode */ }
  });

  /* ---------------------------------------------------------- sidebar */

  var sidebar = doc.getElementById('sidebar');
  var navToggle = doc.getElementById('navToggle');
  var scrim = doc.getElementById('scrim');

  function setNav(open) {
    if (!sidebar) return;
    sidebar.classList.toggle('open', open);
    if (navToggle) navToggle.setAttribute('aria-expanded', open ? 'true' : 'false');
    if (scrim) scrim.hidden = !open;
  }

  if (navToggle) {
    navToggle.addEventListener('click', function () {
      setNav(!sidebar.classList.contains('open'));
    });
  }
  if (scrim) scrim.addEventListener('click', function () { setNav(false); });

  /* -------------------------------------------- heading anchors + rail */

  var content = doc.getElementById('content');
  var rail = doc.getElementById('rail');

  if (content) {
    var heads = content.querySelectorAll('h2[id], h3[id]');
    var railItems = [];

    Array.prototype.forEach.call(heads, function (h) {
      if (!h.querySelector('.anchor')) {
        var a = doc.createElement('a');
        a.className = 'anchor';
        a.href = '#' + h.id;
        a.textContent = '#';
        a.setAttribute('aria-label', 'Link to this section');
        h.appendChild(a);
      }
      railItems.push(h);
    });

    if (rail && railItems.length > 1) {
      var ul = doc.createElement('ul');
      railItems.forEach(function (h) {
        var li = doc.createElement('li');
        var a = doc.createElement('a');
        a.href = '#' + h.id;
        a.textContent = (h.firstChild && h.firstChild.nodeValue ? h.firstChild.nodeValue : h.textContent).trim();
        if (h.tagName === 'H3') a.className = 'lvl3';
        li.appendChild(a);
        ul.appendChild(li);
      });
      rail.appendChild(ul);

      var links = rail.querySelectorAll('a');
      var byId = {};
      Array.prototype.forEach.call(links, function (a) { byId[a.getAttribute('href').slice(1)] = a; });

      if ('IntersectionObserver' in window) {
        var seen = {};
        var obs = new IntersectionObserver(function (entries) {
          entries.forEach(function (en) { seen[en.target.id] = en.isIntersecting; });
          var current = null;
          railItems.forEach(function (h) { if (seen[h.id] && !current) current = h.id; });
          if (!current) return;
          Array.prototype.forEach.call(links, function (a) { a.classList.remove('active'); });
          if (byId[current]) byId[current].classList.add('active');
        }, { rootMargin: '-80px 0px -68% 0px', threshold: 0 });
        railItems.forEach(function (h) { obs.observe(h); });
      }
    }
  }

  /* ----------------------------------------------------------- search */

  var input = doc.getElementById('q');
  var results = doc.getElementById('results');
  var index = window.SIMFLOW_INDEX || [];
  var here = (location.pathname.split('/').pop() || 'index.html');
  var active = -1;

  function esc(s) {
    return String(s).replace(/[&<>"']/g, function (c) {
      return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c];
    });
  }

  function highlight(text, terms) {
    var out = esc(text);
    terms.forEach(function (t) {
      if (t.length < 2) return;
      var re = new RegExp('(' + t.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + ')', 'ig');
      out = out.replace(re, '<mark>$1</mark>');
    });
    return out;
  }

  function score(entry, terms) {
    var title = entry.t.toLowerCase();
    var keys = (entry.k || '').toLowerCase();
    var snip = (entry.s || '').toLowerCase();
    var chap = (entry.c || '').toLowerCase();
    var total = 0;

    for (var i = 0; i < terms.length; i++) {
      var t = terms[i];
      var hit = 0;
      if (title === t) hit += 60;
      if (title.indexOf(t) > -1) hit += 26;
      if (keys.indexOf(t) > -1) hit += 16;
      if (chap.indexOf(t) > -1) hit += 6;
      if (snip.indexOf(t) > -1) hit += 7;
      if (!hit) return 0;
      total += hit;
    }
    if (entry.p === here) total += 4;
    return total;
  }

  function render(list, terms) {
    if (!list.length) {
      results.innerHTML = '<p class="empty">No section matches that. Try a node name, a field name, or a symptom.</p>';
      results.hidden = false;
      return;
    }
    results.innerHTML = list.map(function (e, i) {
      return '<a href="' + e.p + '#' + e.h + '"' + (i === 0 ? ' class="on"' : '') + '>' +
        '<span class="r-t">' + highlight(e.t, terms) + '</span>' +
        '<span class="r-c">' + esc(e.c) + '</span>' +
        '<span class="r-s">' + highlight(e.s, terms) + '</span>' +
        '</a>';
    }).join('');
    results.hidden = false;
    active = 0;
  }

  function run() {
    var raw = input.value.trim().toLowerCase();
    if (raw.length < 2) { results.hidden = true; results.innerHTML = ''; active = -1; return; }
    var terms = raw.split(/\s+/);
    var scored = [];
    for (var i = 0; i < index.length; i++) {
      var s = score(index[i], terms);
      if (s > 0) scored.push({ e: index[i], s: s });
    }
    scored.sort(function (a, b) { return b.s - a.s; });
    render(scored.slice(0, 9).map(function (x) { return x.e; }), terms);
  }

  function move(delta) {
    var items = results.querySelectorAll('a');
    if (!items.length) return;
    if (items[active]) items[active].classList.remove('on');
    active = (active + delta + items.length) % items.length;
    items[active].classList.add('on');
    items[active].scrollIntoView({ block: 'nearest' });
  }

  if (input && results) {
    input.addEventListener('input', run);
    input.addEventListener('focus', function () { if (input.value.trim().length > 1) run(); });

    input.addEventListener('keydown', function (ev) {
      if (ev.key === 'ArrowDown') { ev.preventDefault(); move(1); }
      else if (ev.key === 'ArrowUp') { ev.preventDefault(); move(-1); }
      else if (ev.key === 'Enter') {
        var items = results.querySelectorAll('a');
        if (items[active]) { ev.preventDefault(); location.href = items[active].href; }
      } else if (ev.key === 'Escape') {
        results.hidden = true; input.blur();
      }
    });

    doc.addEventListener('click', function (ev) {
      if (!ev.target.closest('.search')) results.hidden = true;
    });

    doc.addEventListener('keydown', function (ev) {
      var tag = (ev.target.tagName || '').toLowerCase();
      if (ev.key === '/' && tag !== 'input' && tag !== 'textarea') {
        ev.preventDefault();
        input.focus();
        input.select();
      }
    });
  }
})();
