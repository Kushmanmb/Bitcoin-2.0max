(() => {
  "use strict";

  function init() {
    const main = document.getElementById("top");

    if (!main || document.getElementById("networkSearch")) {
      return;
    }

    const networks = {
      ethereum: {
        name: "Ethereum",
        explorer: "https://etherscan.io",
        type: "evm"
      },
      bitcoin: {
        name: "Bitcoin",
        explorer: "https://mempool.space",
        type: "bitcoin"
      },
      base: {
        name: "Base",
        explorer: "https://basescan.org",
        type: "evm"
      },
      arbitrum: {
        name: "Arbitrum One",
        explorer: "https://arbiscan.io",
        type: "evm"
      },
      optimism: {
        name: "Optimism",
        explorer: "https://optimistic.etherscan.io",
        type: "evm"
      },
      polygon: {
        name: "Polygon",
        explorer: "https://polygonscan.com",
        type: "evm"
      },
      bnb: {
        name: "BNB Smart Chain",
        explorer: "https://bscscan.com",
        type: "evm"
      },
      avalanche: {
        name: "Avalanche C-Chain",
        explorer: "https://snowtrace.io",
        type: "evm"
      },
      solana: {
        name: "Solana",
        explorer: "https://explorer.solana.com",
        type: "solana"
      }
    };

    const section = document.createElement("section");
    section.id = "networkSearch";
    section.setAttribute(
      "aria-labelledby",
      "networkSearchTitle"
    );

    section.innerHTML = `
      <div class="network-search-box">
        <div class="network-search-heading">
          <span id="networkSearchTitle">
            MULTINETWORK EXPLORER
          </span>
          <small>Find it. Verify it. Explore it.</small>
        </div>

        <form id="networkSearchForm">
          <label class="network-search-chain">
            Network
            <select id="searchNetwork"></select>
          </label>

          <label class="network-search-query">
            Address, token contract, or transaction hash
            <input
              id="networkSearchInput"
              type="text"
              placeholder="Paste an address or transaction hash…"
              maxlength="160"
              autocomplete="off"
              autocapitalize="off"
              spellcheck="false"
              aria-describedby="networkSearchHelp"
              required
            >
          </label>

          <button type="submit">Search ↗</button>
        </form>

        <p id="networkSearchHelp">
          Choose the correct network. Results open in its
          external explorer; this does not connect your wallet.
        </p>

        <p id="networkSearchStatus" role="status"></p>
        <a
          id="networkSearchResult"
          target="_blank"
          rel="noopener noreferrer"
          hidden
        ></a>
      </div>
    `;

    const menuButton = document.getElementById("navToggle");

if (!menuButton) return;

menuButton.before(section);
document.body.classList.remove("network-search-ready");

    const form = section.querySelector("form");
    const select = section.querySelector("select");
    const input = section.querySelector("input");
    const status = section.querySelector(
      "#networkSearchStatus"
    );
    const result = section.querySelector(
      "#networkSearchResult"
    );

    for (const [key, network] of Object.entries(networks)) {
      select.add(new Option(network.name, key));
    }

    /*
     * Format checks only.
     * The explorer confirms whether the record exists.
     */
    function identify(query, type) {
      if (type === "evm") {
        if (/^0x[a-fA-F0-9]{40}$/.test(query)) {
          return {
            path: "address",
            label: "address / contract",
            value: query
          };
        }

        if (/^0x[a-fA-F0-9]{64}$/.test(query)) {
          return {
            path: "tx",
            label: "transaction",
            value: query
          };
        }

        throw new Error(
          "Enter a 0x address with 40 hexadecimal characters, " +
          "or a 0x transaction hash with 64."
        );
      }

      if (type === "bitcoin") {
        if (/^[a-fA-F0-9]{64}$/.test(query)) {
          return {
            path: "tx",
            label: "transaction",
            value: query.toLowerCase()
          };
        }

        const legacy =
          /^[13][1-9A-HJ-NP-Za-km-z]{25,34}$/.test(query);

        const uniformCase =
          query === query.toLowerCase() ||
          query === query.toUpperCase();

        const bech32 =
          uniformCase &&
          /^bc1[ac-hj-np-z02-9]{11,87}$/i.test(query);

        if (legacy || bech32) {
          return {
            path: "address",
            label: "address",
            value: bech32 ? query.toLowerCase() : query
          };
        }

        throw new Error(
          "Enter a Bitcoin mainnet address or transaction ID. " +
          "Ethereum-style token contracts do not apply to Bitcoin."
        );
      }

      if (type === "solana") {
        if (/^[1-9A-HJ-NP-Za-km-z]{32,44}$/.test(query)) {
          return {
            path: "address",
            label: "account / token mint",
            value: query
          };
        }

        if (/^[1-9A-HJ-NP-Za-km-z]{64,88}$/.test(query)) {
          return {
            path: "tx",
            label: "transaction",
            value: query
          };
        }

        throw new Error(
          "Enter a Solana account, token mint address, " +
          "or transaction signature."
        );
      }

      throw new Error("Unsupported network.");
    }

    function clearResult() {
      status.textContent = "";
      result.hidden = true;
      result.removeAttribute("href");
    }

    input.addEventListener("input", clearResult);
    select.addEventListener("change", clearResult);

    form.addEventListener("submit", event => {
      event.preventDefault();
      clearResult();

      const network = networks[select.value];
      const query = input.value.trim();

      try {
        const match = identify(query, network.type);

        result.href =
          network.explorer + "/" + match.path + "/" +
          encodeURIComponent(match.value);

        result.textContent =
          "Open " + network.name + " " + match.label + " ↗";

        status.textContent =
          "Ready to look up on " + network.name +
          ". Existence and ownership have not been verified.";

        result.hidden = false;
        result.focus();
      } catch (error) {
        status.textContent = error.message;
      }
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init, {
      once: true
    });
  } else {
    init();
  }
})();
