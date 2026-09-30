(() => {
  "use strict";

  if (document.getElementById("btcPriceCard")) return;

  const stats = document.querySelector(".hero__stats");
  if (!stats) return;

  const card = document.createElement("article");
  card.id = "btcPriceCard";
  card.className = "btc-price-card";
  card.setAttribute(
    "aria-label",
    "Bitcoin Mainnet price in US dollars"
  );

  card.innerHTML = `
    <div>
      <div class="section__eyebrow">₿ BITCOIN / USD</div>
      <small>Bitcoin Mainnet · mempool.space</small>
    </div>

    <div class="btc-price-card__quote">
      <strong id="btcPriceValue">—</strong>
      <small id="btcPriceStatus" role="status">
        Loading price…
      </small>
    </div>
  `;

  stats.append(card);

  const value = card.querySelector("#btcPriceValue");
  const status = card.querySelector("#btcPriceStatus");

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

      card.classList.remove("btc-price-card--offline");
    } catch {
      card.classList.add("btc-price-card--offline");

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
})();
