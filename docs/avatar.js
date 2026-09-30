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
        :host { font-family: system-ui, sans-serif; }

        .resident {
          position: absolute;
          width: 72px;
          height: 100px;
          pointer-events: auto;
          touch-action: none;
          user-select: none;
          -webkit-user-select: none;
          cursor: grab;
          will-change: transform;
        }

        .resident.dragging { cursor: grabbing; }

        .resident:focus-visible {
          outline: 2px solid #ffbd66;
          outline-offset: 5px;
          border-radius: 12px;
        }

        .character {
          display: block;
          width: 72px;
          height: 100px;
          overflow: visible;
          pointer-events: none;
          filter: drop-shadow(0 4px 5px #0006);
        }

        .body { transform-origin: 36px 94px; }
        .arm-left { transform-origin: 25px 49px; }
        .arm-right { transform-origin: 47px 49px; }
        .leg-left { transform-origin: 30px 72px; }
        .leg-right { transform-origin: 42px 72px; }

        .bubble {
          position: absolute;
          bottom: 112px;
          left: 50%;
          transform: translateX(-50%);
          width: max-content;
          max-width: 130px;
          padding: 6px 9px;
          border: 1px solid #f7931a;
          border-radius: 12px;
          background: #171717;
          color: #ffd08a;
          font-size: 11px;
          line-height: 1.4;
          text-align: center;
          pointer-events: none;
        }

        .bubble:empty { display: none; }

        .laptop, .board, .parachute, .zip-gear {
          display: none;
        }

        .cable {
          position: absolute;
          inset: 0;
          width: 100%;
          height: 100%;
          overflow: hidden;
          pointer-events: none;
        }

        .cable[hidden] { display: none; }

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

        [data-action="jump"] .arm-left,
        [data-action="handstand"] .arm-left,
        [data-action="cartwheel"] .arm-left,
        [data-action="zipline"] .arm-left {
          transform: rotate(150deg);
        }

        [data-action="jump"] .arm-right,
        [data-action="handstand"] .arm-right,
        [data-action="cartwheel"] .arm-right,
        [data-action="zipline"] .arm-right {
          transform: rotate(-150deg);
        }

        [data-action="wave"] .arm-right {
          animation: wave .45s ease-in-out infinite alternate;
        }

        [data-action="work"] .laptop { display: block; }

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

        [data-action="cartwheel"] .body {
          transform-origin: 36px 50px;
          animation: cartwheel 1.4s linear infinite;
        }

        [data-action="cartwheel"] .leg-left {
          transform: rotate(35deg);
        }

        [data-action="cartwheel"] .leg-right {
          transform: rotate(-35deg);
        }

        [data-action="snowboard"] .board { display: block; }

        [data-action="snowboard"] .body {
          animation: snowboard .8s ease-in-out infinite alternate;
        }

        [data-action="snowboard"] .arm-left {
          transform: rotate(65deg);
        }

        [data-action="snowboard"] .arm-right {
          transform: rotate(-65deg);
        }

        [data-action="parachute"] .parachute {
          display: block;
        }

        [data-action="parachute"] .bubble {
          bottom: 190px;
        }

        [data-action="parachute"] .arm-left {
          transform: rotate(130deg);
        }

        [data-action="parachute"] .arm-right {
          transform: rotate(-130deg);
        }

        [data-action="zipline"] .zip-gear { display: block; }

        .paused *, .dragging * {
          animation-play-state: paused !important;
        }

        @keyframes stride {
          from { transform: rotate(-24deg); }
          to { transform: rotate(24deg); }
        }

        @keyframes squat {
          50% { transform: translateY(12px) scaleY(.85); }
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
          from { transform: translateY(5px) rotate(176deg); }
          to { transform: translateY(5px) rotate(184deg); }
        }

        @keyframes cartwheel {
          from { transform: rotate(0deg); }
          to { transform: rotate(360deg); }
        }

        @keyframes snowboard {
          from { transform: translateY(4px) rotate(-7deg); }
          to { transform: translateY(4px) rotate(7deg); }
        }

        @media (prefers-reduced-motion: reduce) {
          * { animation: none !important; }
        }
      </style>

      <svg class="cable" hidden aria-hidden="true">
        <line stroke="#b8c2d1" stroke-width="3"/>
      </svg>

      <div
        class="resident"
        role="button"
        tabindex="0"
        aria-label="Mini K. Drag to move, or use the arrow keys."
        data-action="wave"
      >
        <div class="bubble" aria-hidden="true"></div>

        <svg class="character" viewBox="0 0 72 100" aria-hidden="true">
          <g class="parachute">
            <path
              d="M-18 -28 Q36 -115 90 -28 Z"
              fill="#f7931a" stroke="#ffd08a" stroke-width="2"
            />
            <path
              d="M9 -28 Q36 -100 63 -28 Z"
              fill="#ffd08a"
            />
            <path
              d="M-18 -28 L25 49 M9 -28 L25 49
                 M63 -28 L47 49 M90 -28 L47 49"
              fill="none" stroke="#e8edf5" stroke-width="2"
            />
            <text
              x="36" y="-37" text-anchor="middle"
              font-size="24" font-weight="bold" fill="#171717"
            >₿</text>
          </g>

          <g class="zip-gear">
            <circle
              cx="36" cy="-10" r="6"
              fill="#f7931a" stroke="#fff" stroke-width="2"
            />
            <path
              d="M36 -4 L36 47"
              stroke="#e8edf5" stroke-width="3"
            />
          </g>

          <g class="body">
            <g class="leg-left">
              <path
                d="M30 71 L27 90"
                stroke="#344054" stroke-width="10"
                stroke-linecap="round"
              />
              <path
                d="M27 91 L20 92"
                stroke="#fff" stroke-width="8"
                stroke-linecap="round"
              />
            </g>

            <g class="leg-right">
              <path
                d="M42 71 L45 90"
                stroke="#344054" stroke-width="10"
                stroke-linecap="round"
              />
              <path
                d="M45 91 L52 92"
                stroke="#fff" stroke-width="8"
                stroke-linecap="round"
              />
            </g>

            <g class="arm-left">
              <path
                d="M25 49 L18 67"
                stroke="#f7931a" stroke-width="9"
                stroke-linecap="round"
              />
              <circle cx="18" cy="70" r="5" fill="#d9a079"/>
            </g>

            <g class="arm-right">
              <path
                d="M47 49 L54 67"
                stroke="#f7931a" stroke-width="9"
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
              fill="none" stroke="#ffd08a" stroke-width="2"
            />
            <text
              x="36" y="67" text-anchor="middle"
              fill="#fff" font-size="20" font-weight="bold"
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
              fill="none" stroke="#fff" stroke-width="2"
              stroke-linecap="round"
            />

            <g class="laptop">
              <rect
                x="17" y="58" width="38" height="23"
                rx="3" fill="#202938" stroke="#ffbd66"
              />
              <text
                x="36" y="74" text-anchor="middle"
                fill="#f7931a" font-size="15"
              >₿</text>
              <path
                d="M14 82 H58"
                stroke="#aeb8c8" stroke-width="4"
                stroke-linecap="round"
              />
            </g>
          </g>

          <g class="board">
            <path
              d="M-8 95 Q-3 105 12 103 H60 Q75 105 80 95"
              fill="#202938" stroke="#ffbd66" stroke-width="4"
              stroke-linecap="round"
            />
            <path
              d="M16 103 H56"
              stroke="#f7931a" stroke-width="3"
            />
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
    const cable = root.querySelector(".cable");
    const cableLine = cable.querySelector("line");
    const reduced = matchMedia("(prefers-reduced-motion: reduce)");

    const messages = {
      walk: "",
      jump: "Parkour!",
      lean: "Just hanging out.",
      workout: "Proof of workout 💪",
      pushups: "One more rep!",
      handstand: "An upside-down outlook.",
      cartwheel: "That's how I roll!",
      snowboard: "Shredding the blockchain!",
      zipline: "Express delivery!",
      parachute: "Special delivery!",
      work: "Checking the blockchain…",
      wave: "Drag me anywhere!"
    };

    const movingActions = new Set([
      "walk", "jump", "cartwheel",
      "snowboard", "zipline", "parachute"
    ]);

    const clamp = (value, min, max) =>
      Math.max(min, Math.min
