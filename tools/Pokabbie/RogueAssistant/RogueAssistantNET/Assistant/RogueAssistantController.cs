﻿using RogueAssistantNET.Assistant.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace RogueAssistantNET.Assistant
{
	public class RogueAssistantController
	{
		public delegate void AssistantCallback(RogueAssistant assistant);

		private bool m_IsRunning = false;
		private TcpListenerServer m_Server;
		private List<RogueAssistant> m_ActiveAssistants = new List<RogueAssistant>();

		private event AssistantCallback m_OnConnect;
		private event AssistantCallback m_OnUpdate;

		public RogueAssistantController()
		{
		}

		public AssistantCallback OnConnect
		{
			get => m_OnConnect;
			set => m_OnConnect = value;
		}

		public AssistantCallback OnUpdate
		{
			get => m_OnUpdate;
			set => m_OnUpdate = value;
		}

		public IEnumerable<RogueAssistant> ActiveAssistants
		{
			get => m_ActiveAssistants;
        }

		public void Run()
		{
			m_IsRunning = true;

			Open();

            while (m_IsRunning)
			{
				Update();
				Thread.Sleep(1000 / 60);
			}
		}

		public void Open()
        {
            Console.WriteLine("Entering main loop..");

            m_Server = new TcpListenerServer(IPAddress.Loopback, 30150);
            m_Server.Open();
        }

		public void Stop()
		{
			m_IsRunning = false;
		}

		public void Update()
		{
			if(m_Server.TryAccept(out TcpClient client))
			{
				if(RogueAssistant.TryAccept(client, out RogueAssistant assistant))
				{
					m_ActiveAssistants.Add(assistant);

					if(m_OnConnect != null)
						m_OnConnect.Invoke(assistant);
				}
				else
				{
					client.Close();
				}
			}

			foreach(var assistant in m_ActiveAssistants)
			{
				if (assistant.HasInitialised)
				{
					if (m_OnUpdate != null)
						m_OnUpdate.Invoke(assistant);
				}

				assistant.Update();
			}
		}

		public void RemoveAssistant(RogueAssistant assistant)
		{
			m_ActiveAssistants.Remove(assistant);
        }
	}
}
