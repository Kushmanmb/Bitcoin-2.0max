(() => {
  "use strict";

  function start() {
    if (document.getElementById("ksmb-resident")) return;

    const host = document.createElement("div");
    host.id = "ksmb-resident";
    host.style.cssText =
      "position:fixed;inset:0;z-index:1000;pointer-events:none;";

    const root = host.attachShadow({ mode: "open" });

    root.innerHTML = `
      <style>
        :host {
          font-family: system-ui, sans-serif;
        }

        .resident {
          position: absolute;
          width: 72px;
          height: 100px;
          pointer-events: none;
          will-change: transform;
        }

        .character {
          display: block;
          width: 72px;
          height: 100px;
          overflow: visible;
          filter: drop-shadow(0 4px 5px #0006);
        }

        .body {
          transform-origin: 36px 94px;
        }

        .arm-left {
          transform-origin: 25px 49px;
        }

        .arm-right {
          transform-origin: 47px 49px;
        }

        .leg-left {
          transform-origin: 30px 72px;
        }

        .leg-right {
          transform-origin: 42px 72px;
        }

        .bubble {
          position: absolute;
          bottom: 106px;
          left: 50%;
          transform: translateX(-50%);
          max-width: 160px;
          width: max-content;
          padding: 6px 9px;
          border: 1px solid #f7931a;
          border-radius: 12px;
          background: #171717;
          color: #ffd08a;
          font-size: 11px;
          line-height: 1.4;
          text-align: center;
        }

        .bubble:empty {
          display: none;
        }

        .laptop {
          display: none;
        }

        [data-action="walk"] .leg-left,
        [data-action="walk"] .arm-right {
          animation: stride .48s ease-in-out infinite alternate;
        }

        [data-action="walk"] .leg-right,
        [data-action="walk"] .arm-left {
          animation: stride .48s ease-in-out infinite alternate-reverse;
        }

        [data-action="workout"] .body {
          animation: squat .9s ease-in-out infinite;
        }

        [data-action="workout"] .arm-left {
          transform: rotate(-75deg);
        }

        [data-action="workout"] .arm-right {
          transform: rotate(75deg);
        }

        [data-action="lean"] .body {
          transform: rotate(14deg);
        }

        [data-action="lean"] .arm-right {
          transform: rotate(-55deg);
        }

        [data-action="jump"] .arm-left {
          transform: rotate(145deg);
        }

        [data-action="jump"] .arm-right {
          transform: rotate(-145deg);
        }

        [data-action="wave"] .arm-right {
          animation: wave .45s ease-in-out infinite alternate;
        }

        [data-action="work"] .laptop {
          display: block;
        }

        [data-action="work"] .arm-left {
          transform: rotate(-50deg);
        }

        [data-action="work"] .arm-right {
          transform: rotate(50deg);
        }

        [data-action="pushups"] .body {
          transform-origin: 36px 50px;
          animation: pushups 1s ease-in-out infinite;
        }

        [data-action="pushups"] .arm-left,
        [data-action="pushups"] .arm-right {
          transform: rotate(-85deg);
        }

        [data-action="handstand"] .body {
          transform-origin: 36px 50px;
          animation: handstand 1.4s ease-in-out infinite alternate;
        }

        [data-action="handstand"] .arm-left {
          transform: rotate(155deg);
        }

        [data-action="handstand"] .arm-right {
          transform: rotate(-155deg);
        }

        [data-action="cartwheel"] .body {
          transform-origin: 36px 50px;
          animation: cartwheel 1.4s linear infinite;
        }

        [data-action="cartwheel"] .arm-left {
          transform: rotate(150deg);
        }

        [data-action="cartwheel"] .arm-right {
          transform: rotate(-150deg);
        }

        [data-action="cartwheel"] .leg-left {
          transform: rotate(35deg);
        }

        [data-action="cartwheel"] .leg-right {
          transform: rotate(-35deg);
        }

        .toggle {
          position: absolute;
          right: 12px;
          bottom: 12px;
          pointer-events: auto;
          padding: 8px 12px;
          border: 1px solid #f7931a;
          border-radius: 20px;
          background: #171717;
          color: #ffbd66;
          cursor: pointer;
          font: inherit;
          font-size: 12px;
        }

        .toggle:focus-visible {
          outline: 3px solid white;
          outline-offset: 3px;
        }

        .paused * {
          animation-play-state: paused !important;
        }

        @keyframes stride {
          from { transform: rotate(-24deg); }
          to { transform: rotate(24deg); }
        }

        @keyframes squat {
          50% {
            transform: translateY(12px) scaleY(.85);
          }
        }

        @keyframes wave {
          from { transform: rotate(-135deg); }
          to { transform: rotate(-165deg); }
        }

        @keyframes pushups {
          0%, 100% {
            transform: translateY(14px) rotate(-90deg);
          }
          50% {
            transform: translateY(22px) rotate(-90deg);
          }
        }

        @keyframes handstand {
          from {
            transform: translateY(5px) rotate(176deg);
          }
          to {
            transform: translateY(5px) rotate(184deg);
          }
        }

        @keyframes cartwheel {
          from { transform: rotate(0deg); }
          to { transform: rotate(360deg); }
        }

        @media (prefers-reduced-motion: reduce) {
          * {
            animation: none !important;
          }
        }
      </style>

      <div class="resident" aria-hidden="true" data-action="wave">
        <div class="bubble"></div>

        <svg class="character" viewBox="0 0 72 100">
          <ellipse
            cx="36" cy="96" rx="21" ry="3"
            fill="#0004"
          />

          <g class="body">
            <g class="leg-left">
              <path
                d="M30 71 L27 90"
                stroke="#344054"
                stroke-width="10"
                stroke-linecap="round"
              />
              <path
                d="M27 91 L20 92"
                stroke="#fff"
                stroke-width="8"
                stroke-linecap="round"
              />
            </g>

            <g class="leg-right">
              <path
                d="M42 71 L45 90"
                stroke="#344054"
                stroke-width="10"
                stroke-linecap="round"
              />
              <path
                d="M45 91 L52 92"
                stroke="#fff"
                stroke-width="8"
                stroke-linecap="round"
              />
            </g>

            <g class="arm-left">
              <path
                d="M25 49 L18 67"
                stroke="#f7931a"
                stroke-width="9"
                stroke-linecap="round"
              />
              <circle cx="18" cy="70" r="5" fill="#d9a079"/>
            </g>

            <g class="arm-right">
              <path
                d="M47 49 L54 67"
                stroke="#f7931a"
                stroke-width="9"
                stroke-linecap="round"
              />
              <circle cx="54" cy="70" r="5" fill="#d9a079"/>
            </g>

            <rect
              x="23" y="43" width="26" height="32"
              rx="9" fill="#f7931a"
            />

            <path
              d="M29 46 L36 52 L43 46"
              fill="none"
              stroke="#ffd08a"
              stroke-width="2"
            />

            <text
              x="36" y="67"
              text-anchor="middle"
              fill="#fff"
              font-size="20"
              font-weight="bold"
            >₿</text>

            <rect
              x="31" y="35" width="10" height="12"
              rx="4" fill="#d9a079"
            />

            <circle cx="36" cy="25" r="16" fill="#d9a079"/>

            <path
              d="M21 25 Q17 7 35 7 Q54 7 51 25
                 L46 18 Q33 22 24 16 Z"
              fill="#30251f"
            />

            <path
              d="M25 32 Q36 46 47 32 Q36 39 25 32"
              fill="#51372b"
            />

            <circle cx="30" cy="26" r="2" fill="#171717"/>
            <circle cx="42" cy="26" r="2" fill="#171717"/>

            <path
              d="M32 32 Q36 35 40 32"
              fill="none"
              stroke="#fff"
              stroke-width="2"
              stroke-linecap="round"
            />

            <g class="laptop">
              <rect
                x="17" y="58" width="38" height="23"
                rx="3" fill="#202938" stroke="#ffbd66"
              />
              <text
                x="36" y="74"
                text-anchor="middle"
                fill="#f7931a"
                font-size="15"
              >₿</text>
              <path
                d="M14 82 H58"
                stroke="#aeb8c8"
                stroke-width="4"
                stroke-linecap="round"
              />
            </g>
          </g>
        </svg>
      </div>

      <button class="toggle" type="button">
        Pause Mini K
      </button>
    `;

    document.body.append(host);

    const resident = root.querySelector(".resident");
    const bubble = root.querySelector(".bubble");
    const toggle = root.querySelector(".toggle");
    const reduced = matchMedia("(prefers-reduced-motion: reduce)");

    const messages = {
      walk: "",
      jump: "Parkour!",
      lean: "Just hanging out.",
      workout: "Proof of workout 💪",
      pushups: "One more rep!",
      handstand: "An upside-down outlook.",
      cartwheel: "That's how I roll!",
      work: "Checking the blockchain…",
      wave: "Hey! I'm Mini K."
    };

    const clamp = (value, min, max) =>
      Math.max(min, Math.min(Math.max(min, max), value));

    let paused = reduced.matches;
    let frame = 0;
    let lastTime = null;
    let elapsed = 0;
    let duration = 3000;
    let action = "wave";
    let destination = { type: "floor", fraction: .15 };
    let position = floorPoint(.15);
    let from = { ...position };

    function floorPoint(fraction) {
      const margin = 24;
      const available = Math.max(0, innerWidth - 72 - margin * 2);

      return {
        x: clamp(
          margin + fraction * available,
          0,
          innerWidth - 72
        ),
        y: Math.max(0, innerHeight - 155)
      };
    }

    function visibleBox() {
      const box = document.querySelector("#txResult");
      if (!box) return null;

      const rect = box.getBoundingClientRect();

      if (
        rect.width < 80 ||
        rect.height === 0 ||
        rect.top < 150 ||
        rect.top > innerHeight - 50 ||
        rect.right < 72 ||
        rect.left > innerWidth - 72
      ) {
        return null;
      }

      return rect;
    }

    function updateAction() {
      resident.dataset.action = action;
      bubble.textContent = messages[action];
    }

    function targetPoint() {
      if (destination.type === "box") {
        const rect = visibleBox();

        if (rect) {
          return {
            x: clamp(
              rect.left + destination.fraction * (rect.width - 72),
              24,
              innerWidth - 96
            ),
            y: rect.top - 96
          };
        }

        // Walk back to the floor if the box scrolls away.
        destination = {
          type: "floor",
          fraction: .1 + Math.random() * .8
        };

        from = { ...position };
        elapsed = 0;
        action = "walk";
        duration = 2500;
        updateAction();
      }

      return floorPoint(destination.fraction);
    }

    function nextAction() {
      from = { ...position };
      elapsed = 0;

      const choices = [
        "walk",
        "workout",
        "pushups",
        "handstand",
        "cartwheel",
        "work",
        "wave"
      ];

      if (visibleBox()) {
        choices.push("jump", "lean");
      }

      // Avoid immediately repeating the same routine.
      const options = choices.filter((choice) => choice !== action);

      action = options[
        Math.floor(Math.random() * options.length)
      ];

      if (action === "jump" || action === "lean") {
        destination = {
          type: "box",
          fraction: .15 + Math.random() * .7,
          afterLanding: action === "lean" ? "lean" : "wave"
        };

        action = "jump";
        duration = 1100;
      } else if (action === "walk" || action === "cartwheel") {
        destination = {
          type: "floor",
          fraction: .1 + Math.random() * .8
        };

        const target = floorPoint(destination.fraction);
        const speed = action === "cartwheel" ? 65 : 85;

        duration = Math.max(
          1800,
          Math.hypot(
            target.x - from.x,
            target.y - from.y
          ) / speed * 1000
        );
      } else {
        duration = 3500 + Math.random() * 3500;
      }

      updateAction();
    }

    function paintPosition() {
      resident.style.transform =
        `translate3d(${position.x}px, ${position.y}px, 0)`;
    }

    function render(time) {
      frame = 0;

      if (paused || document.hidden) {
        lastTime = null;
        return;
      }

      const delta = lastTime === null
        ? 0
        : Math.min(time - lastTime, 50);

      lastTime = time;
      elapsed += delta;

      const target = targetPoint();
      const progress = Math.min(elapsed / duration, 1);

      const moving =
        action === "walk" ||
        action === "jump" ||
        action === "cartwheel";

      if (moving) {
        const arc = action === "jump"
          ? Math.sin(progress * Math.PI) * 85
          : 0;

        position = {
          x: from.x + (target.x - from.x) * progress,
          y: from.y + (target.y - from.y) * progress - arc
        };
      } else {
        // Follow the box's current position while perched.
        position = target;
      }

      position.x = clamp(position.x, 0, innerWidth - 72);
      position.y = clamp(position.y, 0, innerHeight - 100);

      paintPosition();

      if (elapsed >= duration) {
        if (action === "jump") {
          action = destination.afterLanding || "wave";
          elapsed = 0;
          duration = 4000;
          updateAction();
        } else {
          nextAction();
        }
      }

      frame = requestAnimationFrame(render);
    }

    function resumeLoop() {
      if (!paused && !document.hidden && !frame) {
        lastTime = null;
        frame = requestAnimationFrame(render);
      }
    }

    function updatePause() {
      resident.classList.toggle("paused", paused);

      toggle.textContent = paused
        ? "Wake Mini K"
        : "Pause Mini K";

      toggle.setAttribute(
        "aria-label",
        paused ? "Resume avatar animation" : "Pause avatar animation"
      );

      if (paused) {
        cancelAnimationFrame(frame);
        frame = 0;
        lastTime = null;
      } else {
        resumeLoop();
      }
    }

    toggle.addEventListener("click", () => {
      paused = !paused;
      updatePause();
    });

    document.addEventListener("visibilitychange", () => {
      if (document.hidden) {
        cancelAnimationFrame(frame);
        frame = 0;
        lastTime = null;
      } else {
        resumeLoop();
      }
    });

    reduced.addEventListener("change", () => {
      paused = reduced.matches;
      updatePause();
    });

    window.addEventListener("resize", () => {
      position.x = clamp(position.x, 0, innerWidth - 72);
      position.y = clamp(position.y, 0, innerHeight - 100);
      from = { ...position };
      elapsed = 0;
      paintPosition();
    });

    paintPosition();
    updateAction();
    updatePause();
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", start, { once: true });
  } else {
    start();
  }
})();
