const statusMeta = [
  { label: "ยืนตรง", icon: "iconStand", color: "#4ade80" },
  { label: "ล้ม", icon: "iconFall", color: "#ef4444" },
  { label: "เอียงซ้าย", icon: "iconLeanB", color: "#facc15" },
  { label: "เอียงขวา", icon: "iconLeanA", color: "#facc15" },
];
let lastMapKey = "";
const demoMode =
  new URLSearchParams(window.location.search).has("demo") ||
  window.location.protocol === "file:";

function getDemoData() {
  const saved = localStorage.getItem("wildforce-demo-json");
  return saved
    ? JSON.parse(saved)
    : {
        name: "เจ้าหน้าที่อุทยาน02",
        spo2: 92,
        hr: 80,
        status: 0,
        lat: 13.7563,
        lon: 100.5018,
        hasData: true,
        ageSec: 0,
        spo2History: [96, 97, 98],
        hrHistory: [70, 71, 72],
      };
}
function drawGraph(canvas, values, color, minVal, maxVal) {
  const ctx = canvas.getContext("2d"),
    w = canvas.width,
    h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  const padLeft = 30,
    padTop = 10,
    padBottom = 10,
    plotW = w - padLeft - 6,
    plotH = h - padTop - padBottom,
    axisX = Math.round(padLeft) + 0.5;
  ctx.strokeStyle = "rgba(245,245,244,.5)";
  ctx.lineWidth = 3;
  ctx.beginPath();
  ctx.moveTo(axisX, padTop);
  ctx.lineTo(axisX, padTop + plotH);
  ctx.stroke();
  ctx.fillStyle = "rgba(245,245,244,.7)";
  ctx.font = "11px Segoe UI,sans-serif";
  ctx.textAlign = "right";
  ctx.fillText(maxVal, padLeft - 6, padTop + 10);
  ctx.fillText(minVal, padLeft - 6, padTop + plotH);
  if (!values || values.length < 2) return;
  const range = maxVal - minVal || 1;
  ctx.beginPath();
  ctx.strokeStyle = color;
  ctx.lineWidth = 2;
  values.forEach((v, i) => {
    const clamped = Math.max(minVal, Math.min(maxVal, v)),
      x = padLeft + (i / (values.length - 1)) * plotW,
      y = padTop + plotH - ((clamped - minVal) / range) * plotH;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();
}
async function refresh() {
  try {
    const d = demoMode
      ? getDemoData()
      : await fetch("/api/data", { cache: "no-store" }).then((res) =>
          res.json(),
        );
    document.getElementById("name").innerText = d.name;
    document.getElementById("spo2").innerText = d.hasData
      ? d.spo2.toFixed(1) + " %"
      : "--";
    document.getElementById("hr").innerText = d.hasData
      ? d.hr.toFixed(0) + " BPM"
      : "--";
    document.getElementById("rightHr").innerText = d.hasData
      ? d.hr.toFixed(0) + " BPM"
      : "--";
    document.getElementById("rightSpo2").innerText = d.hasData
      ? d.spo2.toFixed(1) + " %"
      : "--";
    const meta = statusMeta[d.status] || statusMeta[0];
    document
      .querySelectorAll(".status-icon")
      .forEach((el) => el.classList.remove("active"));
    const activeIcon = document.getElementById(meta.icon);
    activeIcon.classList.add("active");
    activeIcon.style.color = meta.color;
    const lbl = document.getElementById("statusLabel");
    lbl.innerText = meta.label;
    lbl.style.color = meta.color;
    const staleFlag = d.hasData && d.ageSec > 30;
    document.getElementById("spo2").classList.toggle("stale", staleFlag);
    document.getElementById("hr").classList.toggle("stale", staleFlag);
    document.getElementById("age").innerText = d.hasData
      ? "อัปเดตล่าสุด " + d.ageSec + " วินาทีที่แล้ว"
      : "ยังไม่มีข้อมูล";
    const dangerSpo2 = d.hasData && d.spo2 < 90,
      dangerHr = d.hasData && d.hr < 50;
    document.getElementById("spo2").classList.toggle("danger", dangerSpo2);
    document.getElementById("hr").classList.toggle("danger", dangerHr);
    const banner = document.getElementById("alertBanner");
    if (dangerSpo2 || dangerHr) {
      const msgs = [];
      if (dangerSpo2)
        msgs.push("SpO2 ต่ำกว่า 90% (" + d.spo2.toFixed(1) + "%)");
      if (dangerHr)
        msgs.push("Heart Rate ต่ำกว่า 50 BPM (" + d.hr.toFixed(0) + " BPM)");
      banner.innerText = "แจ้งเตือน: " + msgs.join(" และ ");
      banner.style.display = "block";
    } else banner.style.display = "none";
    document.getElementById("coords").innerText =
      "Lat: " + d.lat.toFixed(5) + " , Lon: " + d.lon.toFixed(5);
    const mapKey = d.lat.toFixed(4) + "," + d.lon.toFixed(4);
    if (mapKey !== lastMapKey && d.lat !== 0 && d.lon !== 0) {
      lastMapKey = mapKey;
      const delta = 0.01;
      document.getElementById("mapFrame").src =
        "https://www.openstreetmap.org/export/embed.html?bbox=" +
        (d.lon - delta) +
        "," +
        (d.lat - delta) +
        "," +
        (d.lon + delta) +
        "," +
        (d.lat + delta) +
        "&layer=mapnik&marker=" +
        d.lat +
        "," +
        d.lon;
    }
  } catch (e) {
    document.getElementById("age").innerText = "Disconnect ESP32.......";
  }
}
refresh();
setInterval(refresh, 2000);
