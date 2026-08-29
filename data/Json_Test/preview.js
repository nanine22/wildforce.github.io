const statusLabels = ["ยืนตรง", "ล้ม", "เอียงซ้าย", "เอียงขวา"];

function setText(id, value) {
  document.getElementById(id).textContent = value;
}

async function loadJson() {
  const response = await fetch("data.json", { cache: "no-store" });
  if (!response.ok) throw new Error("โหลด data.json ไม่สำเร็จ");
  const data = await response.json();
  const hasData = data.hasData !== false;
  const latitude = Number(data.lat) || 0;
  const longitude = Number(data.lon) || 0;

  setText("name", data.name || "--");
  setText("spo2", hasData ? `${Number(data.spo2).toFixed(1)} %` : "--");
  setText("hr", hasData ? `${Number(data.hr).toFixed(0)} BPM` : "--");
  setText("rightSpo2", hasData ? `${Number(data.spo2).toFixed(1)} %` : "--");
  setText("rightHr", hasData ? `${Number(data.hr).toFixed(0)} BPM` : "--");
  setText("status", statusLabels[Number(data.status)] || statusLabels[0]);
  setText(
    "coordinates",
    `Lat: ${latitude.toFixed(5)}, Lon: ${longitude.toFixed(5)}`,
  );

  if (latitude !== 0 && longitude !== 0) {
    const delta = 0.01;
    document.getElementById("map").src =
      `https://www.openstreetmap.org/export/embed.html?bbox=${longitude - delta},${latitude - delta},${longitude + delta},${latitude + delta}&layer=mapnik&marker=${latitude},${longitude}`;
  }
}

loadJson().catch((error) => {
  setText("name", error.message);
  setText("status", "อ่าน JSON ไม่สำเร็จ");
});
