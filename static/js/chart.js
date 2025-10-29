const ctx = document.getElementById('graficoEngajamento').getContext('2d');
const grafico = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        label: 'Engajamento A',
        data: [],
        borderColor: 'rgba(75, 192, 192, 1)',
        backgroundColor: 'rgba(75, 192, 192, 0.2)',
        borderWidth: 2,
        tension: 0.3
      },
      {
        label: 'Engajamento B',
        data: [],
        borderColor: 'rgba(255, 99, 132, 1)',
        backgroundColor: 'rgba(255, 99, 132, 0.2)',
        borderWidth: 2,
        tension: 0.3
      }
    ]
  },
  options: {
    responsive: true,
    plugins: {
      legend: {
        labels: { color: 'white' },
        font: { size: 8 }
      },
    },
    scales: {
      x: {
        ticks: { color: 'white' }
      },
      y: {
        ticks: { color: 'white' },
        beginAtZero: true
      }
    }
  }
});

async function atualizarGraficoEPlacar() {
  const res = await fetch('/dados');
  const data = await res.json();

  // Verifica se o usuário está em um dispositivo móvel
  const isMobile = window.innerWidth < 768;

  // Formata as datas
  grafico.data.labels = data.timestamp.map(ts => {
    const date = new Date(ts);
    return isMobile
      ? date.toLocaleTimeString('pt-BR', { hour12: false }) // mostra só HH:MM:SS
      : date.toLocaleString('pt-BR'); // mostra data completa
  });

  grafico.data.datasets[0].data = data.engj_A;
  grafico.data.datasets[1].data = data.engj_B;
  grafico.update();

  document.getElementById("scoreA").innerText = data.score_A;
  document.getElementById("scoreB").innerText = data.score_B;
}

atualizarGraficoEPlacar();
setInterval(atualizarGraficoEPlacar, 5000);
