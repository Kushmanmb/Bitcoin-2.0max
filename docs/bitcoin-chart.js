(() => {
  "use strict";

  function start() {
    if (document.getElementById("bitcoinHistory")) return;

    const network = document.getElementById("network");
    if (!network) return;

    const section = document.createElement("section");
    section.id = "bitcoinHistory";
    section.className = "section section--alt";
    section.setAttribute("aria-labelledby", "bitcoinHistoryTitle");

    section.innerHTML = `
      <div class="container">
        <div class="section__header">
          <span class="section__eyebrow">
            BITCOIN MAINNET · BTC/USD
          </span>

          <h2 id="bitcoinHistoryTitle" class="section__title">
            Bitcoin price history
          </h2>

          <p class="section__subtitle">
            Bitstamp market prices via TradingView.
            Select All to explore available history.
          </p>
        </div>

        <div
          class="btc-history-chart"
          style="
            height:clamp(420px,65vh,620px);
            width:100%;
            border:1px solid var(--border);
            border-radius:18px;
            overflow:hidden;
            background:var(--surface);
          "
        ></div>
      </div>
    `;

    network.before(section);

    const mount = section.querySelector(".btc-history-chart");
    let currentTheme = null;

    function render() {
      const theme =
        document.documentElement.dataset.theme === "light"
          ? "light"
          : "dark";

      if (theme === currentTheme) return;
      currentTheme = theme;

      const wrapper = document.createElement("div");
      wrapper.className = "tradingview-widget-container";
      wrapper.style.cssText = "height:100%;width:100%";

      wrapper.innerHTML = `
        <div
          class="tradingview-widget-container__widget"
          style="height:calc(100% - 32px);width:100%"
        ></div>

        <div
          class="tradingview-widget-copyright"
          style="height:32px;text-align:center;font-size:12px"
        >
          <a
            href="https://www.tradingview.com/symbols/BTCUSD/?exchange=BITSTAMP"
            target="_blank"
            rel="noopener noreferrer nofollow"
          >
            <span class="blue-text">BTC/USD chart</span>
          </a>
          <span class="trademark"> by TradingView</span>
        </div>
      `;

      mount.replaceChildren(wrapper);

      const script = document.createElement("script");

      script.src =
        "https://s3.tradingview.com/external-embedding/embed-widget-advanced-chart.js";

      script.async = true;

      script.textContent = JSON.stringify({
        autosize: true,
        symbol: "BITSTAMP:BTCUSD",
        interval: "D",
        range: "ALL",
        timezone: "Etc/UTC",
        theme,
        style: "2",
        locale: "en",
        allow_symbol_change: false,
        hide_top_toolbar: false,
        hide_side_toolbar: true,
        hide_volume: true,
        withdateranges: true,
        save_image: true,
        calendar: false,
        support_host: "https://www.tradingview.com"
      });

      script.onerror = () => {
        if (mount.firstElementChild !== wrapper) return;

        const note = document.createElement("p");

        note.textContent =
          "Chart could not load. Open the BTC/USD chart on TradingView below.";

        wrapper.prepend(note);
      };

      wrapper.append(script);
    }

    render();

    new MutationObserver(render).observe(
      document.documentElement,
      {
        attributes: true,
        attributeFilter: ["data-theme"]
      }
    );
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", start, {
      once: true
    });
  } else {
    start();
  }
})();
