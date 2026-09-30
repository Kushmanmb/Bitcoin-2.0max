(() => {
  "use strict";

  const banner = document.getElementById("btcPriceBanner");
  if (!banner) return;

  if (banner.dataset.priceBound === "true") return;
  banner.dataset.priceBound = "true";

  const value = banner.querySelector("#btcPriceValue");
  const status = banner.querySelector("#btcPriceStatus");
  if (!value || !status) return;

  const currency = new Intl.NumberFormat("en-US", {
    style: "currency",
    currency: "USD",
    minimumFractionDigits: 2,
    maximumFractionDigits: 2
  });

  let busy = false;
  let checkedAt = null;

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
        "Checked " +
        checkedAt.toLocaleTimeString([], {
          hour: "2-digit",
          minute: "2-digit",
          second: "2-digit"
        });

      banner.classList.remove("btc-banner--offline");
    } catch {
      banner.classList.add("btc-banner--offline");

      status.textContent = checkedAt
        ? "Feed offline · Last checked " +
          checkedAt.toLocaleTimeString()
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

  function syncBannerHeight() {
    const height = banner.offsetHeight;
    if (height > 0) {
      document.documentElement.style.setProperty(
        "--banner-height",
        height + "px"
      );
    }
  }

  syncBannerHeight();

  if (typeof ResizeObserver === "function") {
    const observer = new ResizeObserver(syncBannerHeight);
    observer.observe(banner);
  } else {
    window.addEventListener("resize", syncBannerHeight);
  }

  window.addEventListener("orientationchange", syncBannerHeight);
})();
