(function () {
  var root = document.documentElement;
  var lines = document.querySelectorAll("#boot .l");
  if (!lines.length) return;
  if (window.matchMedia("(prefers-reduced-motion: reduce)").matches) return;
  root.classList.add("js");
  var i = 0;
  function next() {
    if (i >= lines.length) return;
    lines[i].classList.add("on");
    i += 1;
    setTimeout(next, i === 1 ? 500 : 380);
  }
  next();
})();
