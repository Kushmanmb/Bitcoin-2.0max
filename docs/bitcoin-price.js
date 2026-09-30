const heroStats = document.querySelector(".hero__stats");
if (!heroStats) return;

heroStats.insertAdjacentElement("afterend", banner);
banner.classList.add("btc-price-card");
banner.style.position = "static";
banner.style.inset = "auto";
banner.style.width = "100%";
banner.style.boxSizing = "border-box";
