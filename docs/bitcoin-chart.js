(() => {
  "use strict";

  function start() {
    if (document.getElementById("bitcoinHistory")) return;

    const network = document.getElementById("network");
    if (!network) return;

    const style = document.createElement("style");

    style.textContent = `
      #bitcoinHistory .market-chart-grid {
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: 20px;
      }

      #bitcoinHistory .market-chart-card {
        min-width: 0;
      }

      #bitcoinHistory .market-chart-card h3 {
        margin: 0 0 12px;
      }

      #bitcoinHistory .market-chart-mount {
        height: 480px;
        width: 100%;
        border: 1px solid var(--border);
        border-radius: 16px;
        overflow: hidden;
        background: var(--surface);
      }

      @media (max-width: 900px) {
        #bitcoinHistory .market-chart-grid {
          grid-template-columns: 1fr;
        }

        #bitcoinHistory .market-chart-mount {
          height: 420px;
        }
      }
    `;

    document.head.append(style);

    const section = document.createElement("section");
    section.id = "bitcoinHistory";
    section.className = "section section--alt";
    section.setAttribute("aria-labelledby", "marketChartsTitle");

    section.innerHTML = `
      <div class="container">
        <div class="section__header">
          <span class="section__eyebrow">
            BITCOIN + ETHEREUM · USD
          </span>

          <h2 id="marketChartsTitle" class="section__title">
            Live market charts
          </h2>

          <p class="section__subtitle">
            Bitstamp prices via TradingView.
            Explore each market's available history.
          </p>
        </div>

        <div class="market-chart-grid">
          <article class="market-chart-card">
            <h3>₿ Bitcoin · BTC/USD</h3>
            <div
              class="market-chart-mount"
              data-market="BTCUSD"
            ></div>
          </article>

          <article class="market-chart-card">
            <h3>Ξ Ethereum · ETH/USD</h3>
            <div
              class="market-chart-mount"
              data-market="ETHUSD"
            ></div>
          </article>
        </div>
      </div>
    `;

    network.before(section);

    let currentTheme = null;

    function render() {
      const theme =
        document.documentElement.dataset.theme === "light"
          ? "light"
          : "dark";

      if (theme === currentTheme) return;
      currentTheme = theme;

      section.querySelectorAll("[data-market]").forEach(mount => {
        const symbol = mount.dataset.market;
        const label = symbol === "BTCUSD" ? "BTC/USD" : "ETH/USD";

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
              href="https://www.tradingview.com/symbols/${symbol}/?exchange=BITSTAMP"
              target="_blank"
              rel="noopener noreferrer nofollow"
            >
              <span class="blue-text">${label} chart</span>
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
          symbol: "BITSTAMP:" + symbol,
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
            "Chart unavailable. Use the TradingView link below.";

          wrapper.prepend(note);
        };

        wrapper.append(script);
      });
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
