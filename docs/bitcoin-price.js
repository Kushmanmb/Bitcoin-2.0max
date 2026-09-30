(() => {
  "use strict";

  function initBitcoinPrice() {
    const banner = document.getElementById("btcPriceBanner");
    const heroStats = document.querySelector(".hero__stats");

    if (!banner || !heroStats) return;
    if (banner.dataset.priceBound === "true") return;

    const value = banner.querySelector("#btcPriceValue");
    const status = banner.querySelector("#btcPriceStatus");

    if (!value || !status) return;

    // Restore the price card beneath the hero stats.
    heroStats.insertAdjacentElement("afterend", banner);
    banner.classList.add("btc-price-card");

    Object.assign(banner.style, {
      position: "static",
      inset: "auto",
      width: "100%",
      boxSizing: "border-box"
    });

    // Remove the space reserved for the fixed top banner.
    document.documentElement.style.setProperty(
      "--banner-height",
      "0px"
    );
    document.documentElement.style.setProperty(
      "--btc-banner-height",
      "0px"
    );

    banner.dataset.priceBound = "true";

    const currency = new Intl.NumberFormat("en-US", {
      style: "currency",
      currency: "USD",
      minimumFractionDigits: 2,
      maximumFractionDigits: 2
    });

    let busy = false;
    let checkedAt = null;

    function formatTime(date) {
      return date.toLocaleTimeString([], {
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit"
      });
    }

    async function refreshPrice() {
      if (busy || document.hidden) return;
      busy = true;

      const controller = new AbortController();
      const timeout = setTimeout(
        () => controller.abort(),
        12000
      );

      try {
        const response = await fetch(
          "https://mempool.space/api/v1/prices",
          {
            signal: controller.signal,
            cache: "no-store"
          }
        );

        if (!response.ok) {
          throw new Error("Price feed unavailable");
        }

        const data = await response.json();

        if (
          typeof data.USD !== "number" ||
          !Number.isFinite(data.USD) ||
          data.USD <= 0
        ) {
          throw new Error("Invalid price");
        }

        value.textContent = currency.format(data.USD);
        checkedAt = new Date();

        status.textContent =
          "Checked " + formatTime(checkedAt);

        banner.classList.remove("btc-banner--offline");
      } catch {
        banner.classList.add("btc-banner--offline");

        status.textContent = checkedAt
          ? "Feed offline · Last checked " +
            formatTime(checkedAt)
          : "Price unavailable · Retrying every minute";
      } finally {
        clearTimeout(timeout);
        busy = false;
      }
    }

    refreshPrice();
    setInterval(refreshPrice, 60000);

    document.addEventListener("visibilitychange", () => {
      if (!document.hidden) refreshPrice();
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener(
      "DOMContentLoaded",
      initBitcoinPrice,
      { once: true }
    );
  } else {
    initBitcoinPrice();
  }
})();
