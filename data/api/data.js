let latestData = {
  name: "เจ้าหน้าที่อุทยาน01",
  spo2: 0,
  hr: 0,
  status: 0,
  lat: 0,
  lon: 0,
  hasData: false,
  ageSec: 0,
  spo2History: [],
  hrHistory: [],
  updatedAt: 0,
};

function numberOr(value, fallback) {
  const number = Number(value);
  return Number.isFinite(number) ? number : fallback;
}

module.exports = (request, response) => {
  response.setHeader("Access-Control-Allow-Origin", "*");
  response.setHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  response.setHeader("Access-Control-Allow-Headers", "Content-Type");
  if (request.method === "OPTIONS") return response.status(204).end();

  if (request.method === "POST") {
    const body = request.body || {};
    const spo2 = numberOr(body.spo2, latestData.spo2);
    const hr = numberOr(body.hr, latestData.hr);
    latestData = {
      name: "เจ้าหน้าที่อุทยาน01",
      spo2,
      hr,
      status: Math.max(0, Math.min(3, Math.trunc(numberOr(body.status, 0)))),
      lat: numberOr(body.lat, latestData.lat),
      lon: numberOr(body.lon, latestData.lon),
      hasData: true,
      ageSec: 0,
      spo2History: [...latestData.spo2History, spo2].slice(-20),
      hrHistory: [...latestData.hrHistory, hr].slice(-20),
      updatedAt: Date.now(),
    };
  } else if (request.method !== "GET") {
    return response.status(405).json({ error: "Method not allowed" });
  }

  const ageSec = latestData.updatedAt
    ? Math.floor((Date.now() - latestData.updatedAt) / 1000)
    : 0;
  return response.status(200).json({ ...latestData, ageSec });
};