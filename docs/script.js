/*
 * Bitcoin 2.0max
 * Website Dashboard
 *
 * The dashboard is ready to connect to the Bitcoin 2.0max
 * node API once the backend endpoints are implemented.
 */

(function () {
  "use strict";

  /*
   * ========================================================
   * CONFIGURATION
   * ========================================================
   */

  const CONFIG = {
    // We will change this when our real API is running.
    apiBaseUrl: "",

    refreshInterval: 15000
  };


  /*
   * ========================================================
   * ELEMENTS
   * ========================================================
   */

  const navbar = document.getElementById("navbar");
  const navToggle = document.getElementById("navToggle");
  const navLinks = document.getElementById("navLinks");

  const nodeStatus = document.getElementById("nodeStatus");
  const nodeStatusDetail =
    document.getElementById("nodeStatusDetail");

  const connectionBadge =
    document.getElementById("connectionBadge");

  const blockHeight =
    document.getElementById("blockHeight");

  const peerCount =
    document.getElementById("peerCount");

  const mempoolCount =
    document.getElementById("mempoolCount");

  const heroBlockHeight =
    document.getElementById("heroBlockHeight");

  const heroPeers =
    document.getElementById("heroPeers");

  const heroMempool =
    document.getElementById("heroMempool");

  const blocksTable =
    document.getElementById("blocksTable");

  const transactionsTable =
    document.getElementById("transactionsTable");

  const txSearch =
    document.getElementById("txSearch");

  const txSearchButton =
    document.getElementById("txSearchButton");

  const txResult =
    document.getElementById("txResult");


  /*
   * ========================================================
   * NAVIGATION
   * ========================================================
   */

  function handleScroll() {
    if (!navbar) {
      return;
    }

    if (window.scrollY > 10) {
      navbar.classList.add("scrolled");
    } else {
      navbar.classList.remove("scrolled");
    }
  }


  window.addEventListener(
    "scroll",
    handleScroll,
    { passive: true }
  );


  if (navToggle && navLinks) {

    navToggle.addEventListener(
      "click",
      function () {

        const open =
          navLinks.classList.toggle("open");

        navToggle.classList.toggle(
          "open",
          open
        );

        navToggle.setAttribute(
          "aria-expanded",
          String(open)
        );

      }
    );


    navLinks
      .querySelectorAll("a")
      .forEach(function (link) {

        link.addEventListener(
          "click",
          function () {

            navLinks.classList.remove(
              "open"
            );

            navToggle.classList.remove(
              "open"
            );

            navToggle.setAttribute(
              "aria-expanded",
              "false"
            );

          }
        );

      });

  }


  /*
   * ========================================================
   * HELPERS
   * ========================================================
   */

  function shortenHash(hash) {

    if (!hash) {
      return "—";
    }

    if (hash.length <= 20) {
      return hash;
    }

    return (
      hash.slice(0, 10) +
      "…" +
      hash.slice(-8)
    );

  }


  function formatNumber(value) {

    const number =
      Number(value);

    if (!Number.isFinite(number)) {
      return "—";
    }

    return number.toLocaleString();

  }


  function escapeHTML(value) {

    return String(value)
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#039;");

  }


  function setText(
    element,
    value
  ) {

    if (element) {
      element.textContent = value;
    }

  }


  /*
   * ========================================================
   * NODE CONNECTION STATUS
   * ========================================================
   */

  function setNodeOffline(
    message = "Waiting for node API"
  ) {

    if (nodeStatus) {

      nodeStatus.textContent =
        "OFFLINE";

      nodeStatus.classList.remove(
        "online"
      );

    }


    setText(
      nodeStatusDetail,
      message
    );


    if (connectionBadge) {

      connectionBadge.textContent =
        "NOT CONNECTED";

      connectionBadge.classList.remove(
        "online"
      );

    }

  }


  function setNodeOnline() {

    if (nodeStatus) {

      nodeStatus.textContent =
        "ONLINE";

      nodeStatus.classList.add(
        "online"
      );

    }


    setText(
      nodeStatusDetail,
      "Bitcoin 2.0max node responding"
    );


    if (connectionBadge) {

      connectionBadge.textContent =
        "CONNECTED";

      connectionBadge.classList.add(
        "online"
      );

    }

  }


  /*
   * ========================================================
   * API
   * ========================================================
   */

  async function apiRequest(path) {

    if (!CONFIG.apiBaseUrl) {

      throw new Error(
        "Bitcoin 2.0max API is not configured yet."
      );

    }


    const base =
      CONFIG.apiBaseUrl.replace(
        /\/$/,
        ""
      );


    const response =
      await fetch(
        base + path,
        {
          method: "GET",

          headers: {
            Accept:
              "application/json"
          }
        }
      );


    if (!response.ok) {

      throw new Error(
        "API returned HTTP " +
        response.status
      );

    }


    return response.json();

  }


  /*
   * ========================================================
   * NETWORK STATUS
   * ========================================================
   */

  function renderNetworkStatus(data) {

    setNodeOnline();


    const height =
      formatNumber(
        data.blockheight
      );


    const peers =
      formatNumber(
        data.peers
      );


    const mempool =
      formatNumber(
        data.mempool
      );


    setText(
      blockHeight,
      height
    );


    setText(
      heroBlockHeight,
      height
    );


    setText(
      peerCount,
      peers
    );


    setText(
      heroPeers,
      peers
    );


    setText(
      mempoolCount,
      mempool
    );


    setText(
      heroMempool,
      mempool
    );

  }


  async function loadNetworkStatus() {

    try {

      const data =
        await apiRequest(
          "/status"
        );


      renderNetworkStatus(
        data
      );

    } catch (error) {

      setNodeOffline(
        "Node API not connected"
      );

    }

  }


  /*
   * ========================================================
   * BLOCKS
   * ========================================================
   */

  function renderBlocks(blocks) {

    if (!blocksTable) {
      return;
    }


    if (
      !Array.isArray(blocks) ||
      blocks.length === 0
    ) {

      blocksTable.innerHTML = `
        <tr>
          <td colspan="5">
            <div class="empty-state">
              <span>⛓</span>
              <strong>No blocks available</strong>
              <small>
                Waiting for Bitcoin 2.0max block data.
              </small>
            </div>
          </td>
        </tr>
      `;

      return;

    }


    blocksTable.innerHTML =
      blocks
        .map(function (block) {

          const height =
            formatNumber(
              block.height
            );

          const hash =
            escapeHTML(
              shortenHash(
                block.hash
              )
            );

          const txCount =
            formatNumber(
              block.transactions
            );

          const time =
            escapeHTML(
              block.time || "—"
            );

          const size =
            escapeHTML(
              block.size || "—"
            );


          return `
            <tr>

              <td>
                ${height}
              </td>

              <td>
                <code title="${escapeHTML(block.hash || "")}">
                  ${hash}
                </code>
              </td>

              <td>
                ${txCount}
              </td>

              <td>
                ${time}
              </td>

              <td>
                ${size}
              </td>

            </tr>
          `;

        })
        .join("");

  }


  async function loadBlocks() {

    try {

      const data =
        await apiRequest(
          "/blocks"
        );


      renderBlocks(
        data.blocks || data
      );

    } catch (error) {

      /*
       * Leave the honest waiting message
       * visible until the API exists.
       */

    }

  }


  /*
   * ========================================================
   * RECENT TRANSACTIONS
   * ========================================================
   */

  function renderTransactions(
    transactions
  ) {

    if (!transactionsTable) {
      return;
    }


    if (
      !Array.isArray(
        transactions
      ) ||
      transactions.length === 0
    ) {

      transactionsTable.innerHTML = `
        <tr>
          <td colspan="4">

            <div class="empty-state">

              <span>₿</span>

              <strong>
                No transactions available
              </strong>

              <small>
                Waiting for network transactions.
              </small>

            </div>

          </td>
        </tr>
      `;

      return;

    }


    transactionsTable.innerHTML =
      transactions
        .map(function (tx) {

          const txid =
            escapeHTML(
              shortenHash(
                tx.txid
              )
            );

          const status =
            escapeHTML(
              tx.status ||
              "unknown"
            );

          const block =
            tx.blockHeight ??
            tx.block ??
            "—";

          const value =
            escapeHTML(
              tx.value ??
              "—"
            );


          return `
            <tr>

              <td>
                <code title="${escapeHTML(tx.txid || "")}">
                  ${txid}
                </code>
              </td>

              <td>
                ${status}
              </td>

              <td>
                ${escapeHTML(block)}
              </td>

              <td>
                ${value}
              </td>

            </tr>
          `;

        })
        .join("");

  }


  async function loadTransactions() {

    try {

      const data =
        await apiRequest(
          "/transactions"
        );


      renderTransactions(
        data.transactions ||
        data
      );

    } catch (error) {

      /*
       * API is not available yet.
       */

    }

  }


  /*
   * ========================================================
   * TRANSACTION SEARCH
   * ========================================================
   */

  function showTransactionMessage(
    title,
    message
  ) {

    if (!txResult) {
      return;
    }


    txResult.innerHTML = `
      <div class="empty-state">

        <span>₿</span>

        <strong>
          ${escapeHTML(title)}
        </strong>

        <small>
          ${escapeHTML(message)}
        </small>

      </div>
    `;

  }


  function renderTransaction(
    tx
  ) {

    if (!txResult) {
      return;
    }


    const txid =
      escapeHTML(
        tx.txid || "—"
      );


    const status =
      escapeHTML(
        tx.status ||
        "unknown"
      );


    const block =
      escapeHTML(
        tx.blockHeight ??
        tx.block ??
        "—"
      );


    const value =
      escapeHTML(
        tx.value ??
        "—"
      );


    txResult.innerHTML = `
      <div
        style="
          padding: 24px;
          overflow-wrap: anywhere;
        "
      >

        <div
          style="
            color: var(--orange);
            font-size: .7rem;
            font-weight: 900;
            letter-spacing: .12em;
            margin-bottom: 10px;
          "
        >
          TRANSACTION
        </div>

        <strong>
          ${txid}
        </strong>

        <div
          style="
            display: grid;
            grid-template-columns:
              repeat(
                auto-fit,
                minmax(150px, 1fr)
              );
            gap: 12px;
            margin-top: 20px;
          "
        >

          <div>
            <small>
              STATUS
            </small>
            <br>
            ${status}
          </div>

          <div>
            <small>
              BLOCK
            </small>
            <br>
            ${block}
          </div>

          <div>
            <small>
              VALUE
            </small>
            <br>
            ${value}
          </div>

        </div>

      </div>
    `;

  }


   /*
   * ========================================================
   * DASHBOARD REFRESH
   * ========================================================
   */

  async function refreshDashboard() {

    await Promise.allSettled([
      loadNetworkStatus(),
      loadBlocks(),
      loadTransactions()
    ]);

  }


  /*
   * Initial load
   */

  handleScroll();

  refreshDashboard();


  /*
   * Only poll when an API has
   * actually been configured.
   */

  if (CONFIG.apiBaseUrl) {

    window.setInterval(
      refreshDashboard,
      CONFIG.refreshInterval
    );

  }

})();
/* =========================================================
   LIVE BITCOIN MAINNET DATA
========================================================= */

const BTC_API = "https://mempool.space/api";

async function loadBitcoinData() {
  try {
    const [
      heightResponse,
      mempoolResponse,
      feesResponse,
      priceResponse
    ] = await Promise.all([
      fetch(`${BTC_API}/blocks/tip/height`),
      fetch(`${BTC_API}/mempool`),
      fetch(`${BTC_API}/v1/fees/recommended`),
      fetch(`${BTC_API}/v1/prices`)
    ]);

    if (
      !heightResponse.ok ||
      !mempoolResponse.ok ||
      !feesResponse.ok ||
      !priceResponse.ok
    ) {
      throw new Error("Bitcoin API request failed");
    }

    const height = await heightResponse.text();
    const mempool = await mempoolResponse.json();
    const fees = await feesResponse.json();
    const prices = await priceResponse.json();

    // Existing dashboard elements
    const heroHeight = document.getElementById("heroBlockHeight");
    const blockHeight = document.getElementById("blockHeight");
    const heroMempool = document.getElementById("heroMempool");
    const mempoolCount = document.getElementById("mempoolCount");
    const nodeStatus = document.getElementById("nodeStatus");
    const nodeStatusDetail =
      document.getElementById("nodeStatusDetail");

    if (heroHeight) {
      heroHeight.textContent =
        Number(height).toLocaleString();
    }

    if (blockHeight) {
      blockHeight.textContent =
        Number(height).toLocaleString();
    }

    if (heroMempool) {
      heroMempool.textContent =
        Number(mempool.count).toLocaleString();
    }

    if (mempoolCount) {
      mempoolCount.textContent =
        Number(mempool.count).toLocaleString();
    }

    if (nodeStatus) {
      nodeStatus.textContent = "LIVE";
    }

    if (nodeStatusDetail) {
      nodeStatusDetail.textContent =
        `Bitcoin Mainnet • ${fees.fastestFee} sat/vB • $${Number(
          prices.USD
        ).toLocaleString()} BTC`;
    }

    console.log("Bitcoin Mainnet data loaded:", {
      height,
      mempool: mempool.count,
      fastestFee: fees.fastestFee,
      bitcoinPrice: prices.USD
    });

  } catch (error) {
    console.error("Bitcoin data error:", error);

    const nodeStatus = document.getElementById("nodeStatus");
    const nodeStatusDetail =
      document.getElementById("nodeStatusDetail");

    if (nodeStatus) {
      nodeStatus.textContent = "DATA OFFLINE";
    }

    if (nodeStatusDetail) {
      nodeStatusDetail.textContent =
        "Unable to reach Bitcoin data provider";
    }
  }
}

/* Load immediately */
loadBitcoinData();
loadBitcoinBlocks();
async function loadBitcoinBlocks() {
  const table = document.getElementById("blocksTable");

  if (!table) return;

  try {
    const response = await fetch(`${BTC_API}/blocks`);

    if (!response.ok) {
      throw new Error("Unable to load Bitcoin blocks");
    }

    const blocks = await response.json();

    table.innerHTML = blocks.slice(0, 10).map(block => {

      const time = new Date(
        block.timestamp * 1000
      ).toLocaleString();

      const hash =
        block.id.slice(0, 12) +
        "..." +
        block.id.slice(-8);

      const size =
        (block.size / 1000000).toFixed(2) + " MB";

      return `
        <tr>
          <td>${block.height.toLocaleString()}</td>

          <td title="${block.id}">
            ${hash}
          </td>

          <td>
            ${block.tx_count.toLocaleString()}
          </td>

          <td>${time}</td>

          <td>${size}</td>
        </tr>
      `;
    }).join("");

  } catch (error) {

    console.error(
      "Bitcoin blocks error:",
      error
    );

    table.innerHTML = `
      <tr>
        <td colspan="5">
          Unable to load Bitcoin Mainnet blocks.
        </td>
      </tr>
    `;
  }
}

/* Load immediately */
loadBitcoinData();
loadBitcoinBlocks();

/* Refresh once per minute */
setInterval(() => {
  loadBitcoinData();
  loadBitcoinBlocks();
}, 60000);

/* =========================================================
   LIVE BITCOIN TRANSACTION SEARCH
========================================================= */

async function searchBitcoinTransaction() {
  const input = document.getElementById("txSearch");
  const result = document.getElementById("txResult");

  if (!input || !result) return;

  const txid = input.value.trim();

  if (!/^[a-fA-F0-9]{64}$/.test(txid)) {
    result.innerHTML = `
      <div class="empty-state">
        <span>₿</span>
        <strong>Invalid Bitcoin TXID</strong>
        <small>Enter a 64-character Bitcoin transaction ID.</small>
      </div>
    `;
    return;
  }

  result.innerHTML = `
    <div class="empty-state">
      <span>₿</span>
      <strong>Searching Bitcoin Mainnet...</strong>
    </div>
  `;

  try {
    const response = await fetch(
      `${BTC_API}/tx/${encodeURIComponent(txid)}`
    );

    if (!response.ok) {
      throw new Error("Transaction not found");
    }

    const tx = await response.json();

    const confirmed = tx.status?.confirmed === true;

    const totalOutput = (tx.vout || []).reduce(
      (sum, output) => sum + (output.value || 0),
      0
    );

    const btc = totalOutput / 100000000;

    result.innerHTML = `
      <div style="padding:24px;overflow-wrap:anywhere">

        <div class="section__eyebrow">
          BITCOIN MAINNET TRANSACTION
        </div>

        <p>
          <strong>TXID</strong><br>
          ${tx.txid}
        </p>

        <p>
          <strong>Status</strong><br>
          ${confirmed ? "CONFIRMED" : "UNCONFIRMED"}
        </p>

        <p>
          <strong>Block Height</strong><br>
          ${tx.status?.block_height ?? "Pending"}
        </p>

        <p>
          <strong>Fee</strong><br>
          ${(tx.fee ?? 0).toLocaleString()} sats
        </p>

        <p>
          <strong>Total Outputs</strong><br>
          ${btc.toFixed(8)} BTC
        </p>

        <p>
          <strong>Inputs / Outputs</strong><br>
          ${(tx.vin || []).length} / ${(tx.vout || []).length}
        </p>

      </div>
    `;

  } catch (error) {

    console.error("Bitcoin transaction error:", error);

    result.innerHTML = `
      <div class="empty-state">
        <span>₿</span>
        <strong>Transaction not found</strong>
        <small>
          Check the TXID and try again.
        </small>
      </div>
    `;
  }
}

/* Connect existing transaction-search controls */
const bitcoinTxButton =
  document.getElementById("txSearchButton");

const bitcoinTxInput =
  document.getElementById("txSearch");

if (bitcoinTxButton) {
  bitcoinTxButton.addEventListener(
    "click",
    searchBitcoinTransaction
  );
}

if (bitcoinTxInput) {
  bitcoinTxInput.addEventListener(
    "keydown",
    event => {
      if (event.key === "Enter") {
        searchBitcoinTransaction();
      }
    }
  );
}
