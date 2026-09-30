(() => {
  "use strict";

  const RPC = "https://ethereum-rpc.publicnode.com";
  let id = 0;

  async function rpc(method, params = []) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 12000);
    const requestId = ++id;

    try {
      const response = await fetch(RPC, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          jsonrpc: "2.0", id: requestId, method, params
        }),
        signal: controller.signal
      });

      if (!response.ok) throw new Error("Provider unavailable.");

      const data = await response.json();

      if (data.error || data.id !== requestId || data.result == null) {
        throw new Error("Invalid RPC response.");
      }

      return data.result;
    } finally {
      clearTimeout(timer);
    }
  }

  function units(value, decimals) {
    const amount = BigInt(value);
    const scale = 10n ** BigInt(decimals);
    const fraction = String(amount % scale)
      .padStart(decimals, "0").replace(/0+$/, "");

    return String(amount / scale) + (fraction ? "." + fraction : "");
  }

  async function verifyMainnet() {
    if (BigInt(await rpc("eth_chainId")) !== 1n) {
      throw new Error("Not Ethereum Mainnet.");
    }
  }

  function start() {
    if (document.getElementById("ethereumNetwork")) return;

    const network = document.getElementById("network");
    if (!network) return;

    const panel = document.createElement("section");
    panel.id = "ethereumNetwork";
    panel.className = "section section--alt";
    panel.setAttribute("aria-labelledby", "ethereumTitle");

    panel.innerHTML = `
      <div class="container">
        <div class="section__header">
          <span class="section__eyebrow">
            ETHEREUM MAINNET · CHAIN ID 1
          </span>
          <h2 class="section__title" id="ethereumTitle">
            Ethereum Mainnet
          </h2>
          <p class="section__subtitle">
            PublicNode data · refreshes every 30 seconds.
          </p>
          <p data-eth="status" role="status">Connecting…</p>
        </div>

        <div class="dashboard-grid">
          <article class="dashboard-card">
            <h3>Latest block</h3>
            <div class="dashboard-value" data-eth="height">—</div>
          </article>
          <article class="dashboard-card">
            <h3>Suggested gas price</h3>
            <div class="dashboard-value" data-eth="gas">—</div>
            <small>Gwei per unit of gas</small>
          </article>
          <article class="dashboard-card">
            <h3>Transactions in latest block</h3>
            <div class="dashboard-value" data-eth="transactions">—</div>
          </article>
        </div>

        <h3 style="margin-top:32px">Ethereum address lookup</h3>
        <form class="tx-search">
          <input type="text" placeholder="0x Ethereum address"
            aria-label="Ethereum Mainnet address"
            required autocomplete="off" spellcheck="false">
          <button class="btn btn--primary" type="submit">
            Check balance
          </button>
        </form>
        <div class="lookup-panel" data-eth="balance"
          role="status" style="overflow-wrap:anywhere"></div>
      </div>
    `;

    network.after(panel);

    const field = name =>
      panel.querySelector('[data-eth="' + name + '"]');

    let busy = false;

    async function refresh() {
      if (busy || document.hidden) return;
      busy = true;

      try {
        await verifyMainnet();

        const [block, gas] = await Promise.all([
          rpc("eth_getBlockByNumber", ["latest", false]),
          rpc("eth_gasPrice")
        ]);

        if (!block.number || !Array.isArray(block.transactions)) {
          throw new Error("Unexpected block response.");
        }

        field("height").textContent =
          BigInt(block.number).toLocaleString();
        field("gas").textContent = units(gas, 9);
        field("transactions").textContent =
          block.transactions.length.toLocaleString();
        field("status").textContent =
          "LIVE · Updated " + new Date().toLocaleTimeString();
      } catch {
        field("status").textContent =
          "DATA OFFLINE · Previous values may be outdated.";
      } finally {
        busy = false;
      }
    }

    const form = panel.querySelector("form");
    const input = form.querySelector("input");
    const button = form.querySelector("button");

    form.addEventListener("submit", async event => {
      event.preventDefault();
      if (button.disabled) return;

      const address = input.value.trim();
      const result = field("balance");

      if (!/^0x[0-9a-fA-F]{40}$/.test(address)) {
        result.textContent =
          "Enter a 0x address with 40 hexadecimal characters.";
        return;
      }

      button.disabled = true;
      result.textContent = "Checking Ethereum Mainnet…";

      try {
        await verifyMainnet();

        const balance = await rpc(
          "eth_getBalance", [address, "latest"]
        );

        const text = document.createElement("p");
        text.textContent = address + " · Native ETH balance: " +
          units(balance, 18) + " ETH";

        const link = document.createElement("a");
        link.href = "https://etherscan.io/address/" + address;
        link.target = "_blank";
        link.rel = "noopener noreferrer";
        link.textContent =
          "View address and token balances on Etherscan ↗";

        result.replaceChildren(text, link);
      } catch {
        result.textContent =
          "Unable to retrieve balance. Please try again.";
      } finally {
        button.disabled = false;
      }
    });

    refresh();
    setInterval(refresh, 30000);

    document.addEventListener("visibilitychange", () => {
      if (!document.hidden) refresh();
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", start, { once: true });
  } else {
    start();
  }
})();
