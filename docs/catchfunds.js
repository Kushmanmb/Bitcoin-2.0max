const CATCHFUNDS = {
  bitcoin: {
    symbol: "BTC",
    address: "1482Dhe2GhnkocFndFh4wk7Ke3Aa2i3KZD"
  },

  bitcoinCash: {
    symbol: "BCH",
    address: "bitcoincash:qqddagx2ms4qgczgwu0qryxpq9p8ye8fvy8exe3kpu"
  },

  stellar: {
    symbol: "XLM",
    address: "GCSFVENICIUKIBGVR72JZ3NIFT64XH7EXCYFMUWUJZC47YDXJFPPWBKS"
  },

  evm: {
    address: "0xabF0FA14EF084ADdaD0F33ADD0B1bad6a43019B5",

    networks: [
      "Ethereum",
      "Polygon",
      "Arbitrum One",
      "Avalanche C-Chain",
      "BNB Smart Chain"
    ],

    trackedTokens: [
      "VERSE — Ethereum",
      "VERSE — Polygon"
    ]
  }
};
/* =========================================================
   CATCHFUNDS DASHBOARD
========================================================= */

function renderCatchFunds() {
  const dashboard =
    document.getElementById("catchfundsDashboard");

  if (!dashboard) return;

  const wallets = [
  {
    name: "Bitcoin",
    badge: "btc",
    symbol: CATCHFUNDS.bitcoin.symbol,
    network: "Bitcoin Mainnet",
    address: CATCHFUNDS.bitcoin.address
  },
  {
    name: "Bitcoin Cash",
    badge: "bch",
    symbol: CATCHFUNDS.bitcoinCash.symbol,
    network: "Bitcoin Cash",
    address: CATCHFUNDS.bitcoinCash.address
  },
  {
    name: "Stellar",
    badge: "xlm",
    symbol: CATCHFUNDS.stellar.symbol,
    network: "Stellar",
    address: CATCHFUNDS.stellar.address
  },
  {
    name: "EVM CatchFunds",
    badge: "evm",
    symbol: "EVM",
    network: CATCHFUNDS.evm.networks.join(" • "),
    address: CATCHFUNDS.evm.address
  }
];

  dashboard.innerHTML = wallets.map(wallet => `
    <article class="catchfunds-card catchfunds-card--${wallet.badge}">

      <div class="catchfunds-card__header">
        <strong>${wallet.name}</strong>
        <span class="network-badge network-badge--${wallet.badge}">
  ${wallet.symbol}
</span>
      </div>

      <small>${wallet.network}</small>

      <code class="catchfunds-address">
        ${wallet.address}
      </code>

      <button
        class="btn btn--ghost catchfunds-copy"
        type="button"
        data-address="${wallet.address}"
      >
        Copy Address
      </button>

    </article>
  `).join("");
}

document.addEventListener("click", async event => {
  const button =
    event.target.closest(".catchfunds-copy");

  if (!button) return;

  const address = button.dataset.address;

  try {
    await navigator.clipboard.writeText(address);

    const original = button.textContent;
    button.textContent = "Copied ✓";

    setTimeout(() => {
      button.textContent = original;
    }, 1500);

  } catch (error) {
    console.error("Unable to copy address:", error);
  }
});

renderCatchFunds();
